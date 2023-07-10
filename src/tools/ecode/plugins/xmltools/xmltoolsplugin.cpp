#include "xmltoolsplugin.hpp"
#include <eepp/graphics/primitives.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/scopedop.hpp>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
#if EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN || defined( __EMSCRIPTEN_PTHREADS__ )
#define XMLTOOLS_THREADED 1
#else
#define XMLTOOLS_THREADED 0
#endif

namespace ecode {

UICodeEditorPlugin* XMLToolsPlugin::New( PluginManager* pluginManager ) {
	return eeNew( XMLToolsPlugin, ( pluginManager, false ) );
}

UICodeEditorPlugin* XMLToolsPlugin::NewSync( PluginManager* pluginManager ) {
	return eeNew( XMLToolsPlugin, ( pluginManager, true ) );
}

XMLToolsPlugin::XMLToolsPlugin( PluginManager* pluginManager, bool sync ) :
	PluginBase( pluginManager ) {
	if ( sync ) {
		load( pluginManager );
	} else {
#if FORMATTER_THREADED
		mThreadPool->run( [&, pluginManager] { load( pluginManager ); } );
#else
		load( pluginManager );
#endif
	}
}

XMLToolsPlugin::~XMLToolsPlugin() {
	mShuttingDown = true;
	{
		Lock l( mClientsMutex );
		for ( const auto& client : mClients )
			client.first->unregisterClient( client.second.get() );
	}
}

bool XMLToolsPlugin::getHighlightMatch() const {
	return mHighlightMatch;
}

bool XMLToolsPlugin::getAutoEditMatch() const {
	return mAutoEditMatch;
}

void XMLToolsPlugin::load( PluginManager* pluginManager ) {
	BoolScopedOp loading( mLoading, true );
	std::string path = pluginManager->getPluginsPath() + "xmltools.json";
	if ( FileSystem::fileExists( path ) ||
		 FileSystem::fileWrite( path, "{\n  \"config\":{},\n  \"keybindings\":{}\n}\n" ) ) {
		mConfigPath = path;
	}
	std::string data;
	if ( !FileSystem::fileGet( path, data ) )
		return;
	mConfigHash = String::hash( data );

	json j;
	try {
		j = json::parse( data, nullptr, true, true );
	} catch ( const json::exception& e ) {
		Log::error( "XMLToolsPlugin::load - Error parsing config from path %s, error: %s, config "
					"file content:\n%s",
					path.c_str(), e.what(), data.c_str() );
		// Recreate it
		j = json::parse( "{\n  \"config\":{},\n  \"keybindings\":{},\n}\n", nullptr, true, true );
	}

	bool updateConfigFile = false;

	if ( j.contains( "config" ) ) {
		auto& config = j["config"];
		if ( config.contains( "highlight_match" ) )
			mHighlightMatch = config.value( "highlight_match", true );
		else {
			config["highlight_match"] = mHighlightMatch;
			updateConfigFile = true;
		}
		if ( config.contains( "auto_edit_match" ) )
			mAutoEditMatch = config.value( "auto_edit_match", true );
		else {
			config["auto_edit_match"] = mAutoEditMatch;
			updateConfigFile = true;
		}
	}

	if ( updateConfigFile ) {
		data = j.dump( 2 );
		FileSystem::fileWrite( path, data );
		mConfigHash = String::hash( data );
	}

	fireReadyCbs();
	subscribeFileSystemListener();
}

void XMLToolsPlugin::onRegisterDocument( TextDocument* doc ) {
	Lock l( mClientsMutex );
	mClients[doc] = std::make_unique<XMLToolsClient>( this, doc );
	doc->registerClient( mClients[doc].get() );
}

void XMLToolsPlugin::onUnregisterDocument( TextDocument* doc ) {
	Lock l( mClientsMutex );
	doc->unregisterClient( mClients[doc].get() );
	mClients.erase( doc );
}

bool XMLToolsPlugin::isOverMatch( TextDocument* doc, const Int64& index ) const {
	if ( mMatches.empty() )
		return false;
	auto clientIt = mMatches.find( doc );
	if ( clientIt == mMatches.end() )
		return false;
	const ClientMatch& match = clientIt->second;
	if ( match.matchBracket.start().line() != index &&
		 match.currentBracket.start().line() != index )
		return false;
	if ( !match.matchBracket.inSameLine() && !match.currentBracket.inSameLine() )
		return false;
	return true;
}

static bool isClosedTag( TextDocument* doc, TextPosition start ) {
	SyntaxHighlighter* highlighter = doc->getHighlighter();
	TextPosition endOfDoc = doc->endOfDoc();
	String::StringBaseType prevChar = '\0';
	do {
		String::StringBaseType ch = doc->getChar( start );
		if ( ch == '>' ) {
			auto tokenType = highlighter->getTokenTypeAt( start );
			if ( tokenType != "comment" && tokenType != "string" )
				return prevChar == '/';
		}
		start = doc->positionOffset( start, 1 );
		prevChar = ch;
	} while ( start.isValid() && start != endOfDoc );
	return false;
}

void XMLToolsPlugin::XMLToolsClient::onDocumentTextChanged(
	const DocumentContentChange& docChange ) {
	if ( mAutoInserting || !mParent->getAutoEditMatch() ||
		 !mParent->isOverMatch( mDoc, docChange.range.start().line() ) ||
		 mDoc->getSelections().size() > 1 )
		return;
	ClientMatch& match = mParent->mMatches[mDoc];
	if ( !match.currentBracket.contains( docChange.range ) )
		return;
	mAutoInserting = true;
	auto sel = mDoc->getSelections();
	auto diff = docChange.range.start() - match.currentBracket.start() +
				( match.currentIsClose ? TextPosition( 0, 0 ) : TextPosition( 0, 1 ) );
	auto translatedPos = match.matchBracket.normalize().start() + diff;
	if ( match.currentIsClose ) {
		translatedPos = mDoc->positionOffset( translatedPos, -1 );
	}
	mDoc->setSelection( 0, translatedPos );
	auto translation =
		docChange.range.normalized().end().column() - docChange.range.normalized().start().column();
	if ( docChange.text.empty() ) {
		if ( match.currentBracket.start().line() == match.matchBracket.start().line() ) {
			if ( !match.currentIsClose ) {
				translatedPos = mDoc->positionOffset( translatedPos, -translation );
			}
		}
		mDoc->remove(
			0, { translatedPos, { translatedPos.line(), translatedPos.column() + translation } } );
		if ( mDoc->isInsertingText() ) {
			mWaitingText = true;
		} else {
			TextRange range = match.currentIsClose ? match.currentBracket : match.matchBracket;
			range.normalize();
			if ( match.isSameLine() && !match.currentIsClose ) {
				range.setStart( mDoc->positionOffset( range.start(), -translation + 1 ) );
			}
			auto closeText =
				mDoc->getText( { range.start(), mDoc->positionOffset( range.start(), 3 ) } );
			if ( closeText == "</>" ) {
				mJustDeletedWholeWord = true;
				if ( match.isSameLine() ) {
					match.currentBracket = { match.currentBracket.start(),
											 mDoc->positionOffset( match.currentBracket.start(),
																   match.currentIsClose ? 2 : 1 ) };
					match.matchBracket = {
						match.matchBracket.start(),
						mDoc->positionOffset( match.matchBracket.start(),
											  match.currentIsClose ? 1 : 1 - translation ) };
				} else {
					match.currentBracket = { match.currentBracket.start(),
											 mDoc->positionOffset( match.currentBracket.start(),
																   match.currentIsClose ? 2 : 1 ) };
					match.matchBracket = { match.matchBracket.start(),
										   mDoc->positionOffset( match.matchBracket.start(),
																 match.currentIsClose ? 1 : 2 ) };
				}
			}
		}
	} else {
		if ( match.isSameLine() && !match.currentIsClose ) {
			translatedPos =
				mDoc->positionOffset( translatedPos, translation + docChange.text.size() );
		}
		mDoc->insert( 0, translatedPos, docChange.text );
		mWaitingText = false;
		if ( match.isSameLine() && match.currentIsClose ) {
			for ( auto& s : sel ) {
				s.start().setColumn( s.start().column() + docChange.text.size() * 2 + 1 );
				s.end().setColumn( s.end().column() + docChange.text.size() * 2 + 1 );
			}
			mForceSelections = true;
			mSelections = sel;
		}
	}
	if ( !mJustDeletedWholeWord )
		mAutoInserting = false;
	mDoc->setSelection( sel );
	mAutoInserting = false;
}

void XMLToolsPlugin::XMLToolsClient::updateMatch( const TextRange& sel ) {
	const auto& line = mDoc->line( mDoc->getSelection().start().line() ).getText();
	if ( mDoc->getSelection().start().column() >= (Int64)line.size() )
		return clearMatch();
	auto def = mDoc->getHighlighter()->getSyntaxDefinitionFromTextPosition( sel.start() );
	if ( !def.getAutoCloseXMLTags() ) // getAutoCloseXMLTags means that it supports XML element tags
		return clearMatch();
	TextRange range = mDoc->getWordRangeInPosition( sel.start(), false );
	if ( !range.isValid() )
		return clearMatch();
	range.normalize();
	if ( range.start().column() == 0 || line.size() <= 1 )
		return clearMatch();
	if ( line[range.start().column() - 1] != '<' && line[range.start().column() - 1] != '/' &&
		 ( range.start().column() - 2 < 0 || range.start().column() - 2 >= (Int64)line.size() ||
		   line[range.start().column() - 2] != '<' ) )
		return clearMatch();
	bool isCloseBracket = line[range.start().column() - 1] == '/';
	if ( !isCloseBracket && isClosedTag( mDoc, range.end() ) )
		return clearMatch();
	range.start().setColumn( range.start().column() - ( isCloseBracket ? 2 : 1 ) );
	if ( mParent->mMatches.count( mDoc ) > 0 ) {
		const ClientMatch& curMatch( mParent->mMatches[mDoc] );
		if ( curMatch.currentBracket == range )
			return; // Moving inside match
	}
	String tag( mDoc->getText( range ) );

	TextRange found;
	if ( isCloseBracket ) {
		String openBracket( tag );
		openBracket.erase( 1 );
		found = mDoc->getMatchingBracket( range.start(), openBracket, tag,
										  TextDocument::MatchDirection::Backward );
	} else {
		String closeBracket( tag );
		closeBracket.insert( 1, '/' );
		found = mDoc->getMatchingBracket( range.start(), tag, closeBracket,
										  TextDocument::MatchDirection::Forward );
	}

	if ( found.isValid() ) {
		ClientMatch match{ range, found, isCloseBracket };
		mParent->mMatches[mDoc] = std::move( match );
	} else {
		clearMatch();
	}
}

void XMLToolsPlugin::XMLToolsClient::onDocumentSelectionChange( const TextRange& sel ) {
	if ( mForceSelections ) {
		mDoc->setSelection( mSelections );
		mForceSelections = false;
	}
	if ( mAutoInserting || mWaitingText )
		return;
	if ( mJustDeletedWholeWord ) {
		mJustDeletedWholeWord = false;
		return;
	}
	if ( !mParent->getHighlightMatch() && !mParent->getAutoEditMatch() )
		return clearMatch();
	if ( mDoc->getSelection().start().line() >= (Int64)mDoc->linesCount() )
		return clearMatch();
	updateMatch( sel );
}

void XMLToolsPlugin::XMLToolsClient::clearMatch() {
	if ( !mParent->mMatches.empty() )
		mParent->mMatches.erase( mDoc );
}

void XMLToolsPlugin::drawBeforeLineText( UICodeEditor* editor, const Int64& index,
										 Vector2f position, const Float& /*fontSize*/,
										 const Float& lineHeight ) {
	if ( !isOverMatch( &editor->getDocument(), index ) )
		return;
	Primitives p;
	Color color( editor->getColorScheme().getEditorSyntaxStyle( "matching_bracket" ).color );
	Color blendedColor( Color( color, 50 ) );
	p.setColor( blendedColor );

	const ClientMatch& match = mMatches[&editor->getDocument()];
	for ( const auto& range : { match.matchBracket, match.currentBracket } ) {
		if ( range.start().line() != index || !range.inSameLine() )
			continue;
		Float offset1 = editor->getXOffsetCol( range.normalized().start() );
		Float offset2 = editor->getXOffsetCol( range.normalized().end() );
		p.drawRectangle(
			Rectf( { position.x + offset1, position.y }, { ( offset2 - offset1 ), lineHeight } ) );
	}
}

void XMLToolsPlugin::minimapDrawAfterLineText( UICodeEditor* editor, const Int64& index,
											   const Vector2f& pos, const Vector2f& size,
											   const Float&, const Float& ) {
	if ( !isOverMatch( &editor->getDocument(), index ) )
		return;
	Primitives p;
	Color color( editor->getColorScheme().getEditorSyntaxStyle( "matching_bracket" ).color );
	Color blendedColor( Color( color, 50 ) );
	p.setColor( blendedColor );

	const ClientMatch& match = mMatches[&editor->getDocument()];
	for ( const auto& range : { match.matchBracket, match.currentBracket } ) {
		if ( range.start().line() != index || !range.inSameLine() )
			continue;
		p.drawRectangle( Rectf( pos, size ) );
	}
}

} // namespace ecode
