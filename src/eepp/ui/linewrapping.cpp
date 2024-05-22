#include <eepp/graphics/text.hpp>
#include <eepp/system/luapattern.hpp>
#include <eepp/system/scopedop.hpp>
#include <eepp/ui/doc/syntaxhighlighter.hpp>
#include <eepp/ui/linewrapping.hpp>

namespace EE { namespace UI {

LineWrapMode LineWrapping::toLineWrapMode( std::string mode ) {
	String::toLowerInPlace( mode );
	if ( mode == "word" )
		return LineWrapMode::Word;
	if ( mode == "letter" )
		return LineWrapMode::Letter;
	return LineWrapMode::NoWrap;
}

std::string LineWrapping::fromLineWrapMode( LineWrapMode mode ) {
	switch ( mode ) {
		case LineWrapMode::Letter:
			return "letter";
		case LineWrapMode::Word:
			return "word";
		case LineWrapMode::NoWrap:
		default:
			return "nowrap";
	}
}

LineWrapType LineWrapping::toLineWrapType( std::string type ) {
	String::toLowerInPlace( type );
	if ( "line_breaking_column" == type )
		return LineWrapType::LineBreakingColumn;
	return LineWrapType::Viewport;
}

std::string LineWrapping::fromLineWrapType( LineWrapType type ) {
	switch ( type ) {
		case LineWrapType::LineBreakingColumn:
			return "line_breaking_column";
		case LineWrapType::Viewport:
		default:
			return "viewport";
	}
}

Float LineWrapping::computeOffsets( const String& string, const FontStyleConfig& fontStyle,
									Uint32 tabWidth ) {

	auto nonIndentPos = string.find_first_not_of( " \t\n\v\f\r" );
	if ( nonIndentPos != String::InvalidPos )
		return Text::getTextWidth( string.view().substr( 0, nonIndentPos ), fontStyle, tabWidth );
	return 0.f;
}

LineWrapping::LineWrapInfo LineWrapping::computeLineBreaks( const String& string,
															const FontStyleConfig& fontStyle,
															Float maxWidth, LineWrapMode mode,
															bool keepIndentation,
															Uint32 tabWidth ) {
	LineWrapInfo info;
	info.wraps.push_back( 0 );
	if ( string.empty() || nullptr == fontStyle.Font || mode == LineWrapMode::NoWrap )
		return info;

	if ( keepIndentation ) {
		auto nonIndentPos = string.find_first_not_of( " \t\n\v\f\r" );
		if ( nonIndentPos != String::InvalidPos )
			info.paddingStart =
				Text::getTextWidth( string.view().substr( 0, nonIndentPos ), fontStyle, tabWidth );
	}

	Float xoffset = 0.f;
	Float lastWidth = 0.f;
	bool bold = ( fontStyle.Style & Text::Style::Bold ) != 0;
	bool italic = ( fontStyle.Style & Text::Style::Italic ) != 0;
	bool isMonospace = fontStyle.Font->isMonospace();
	Float outlineThickness = fontStyle.OutlineThickness;

	auto tChar = &string[0];
	size_t lastSpace = 0;
	Uint32 prevChar = 0;

	Float hspace = static_cast<Float>(
		fontStyle.Font->getGlyph( L' ', fontStyle.CharacterSize, bold, italic, outlineThickness )
			.advance );
	size_t idx = 0;

	while ( *tChar ) {
		Uint32 curChar = *tChar;
		Float w = !isMonospace ? fontStyle.Font
									 ->getGlyph( curChar, fontStyle.CharacterSize, bold, italic,
												 outlineThickness )
									 .advance
							   : hspace;

		if ( curChar == '\t' )
			w += hspace * tabWidth;
		else if ( ( curChar ) == '\r' )
			w = 0;

		if ( !isMonospace && curChar != '\r' ) {
			w += fontStyle.Font->getKerning( prevChar, curChar, fontStyle.CharacterSize, bold,
											 italic, outlineThickness );
			prevChar = curChar;
		}

		xoffset += w;

		if ( xoffset > maxWidth ) {
			if ( mode == LineWrapMode::Word && lastSpace ) {
				info.wraps.push_back( lastSpace + 1 );
				xoffset = w + info.paddingStart + ( xoffset - lastWidth );
			} else {
				info.wraps.push_back( idx );
				xoffset = w + info.paddingStart;
			}
			lastSpace = 0;
		} else if ( curChar == ' ' || curChar == '.' || curChar == '-' || curChar == ',' ) {
			lastSpace = idx;
			lastWidth = xoffset;
		}

		idx++;
		tChar++;
	}

	return info;
}

LineWrapping::LineWrapInfo LineWrapping::computeLineBreaks( const TextDocument& doc, size_t line,
															const FontStyleConfig& fontStyle,
															Float maxWidth, LineWrapMode mode,
															bool keepIndentation,
															Uint32 tabWidth ) {
	return computeLineBreaks( doc.line( line ).getText(), fontStyle, maxWidth, mode,
							  keepIndentation, tabWidth );
}

LineWrapping::LineWrapping( std::shared_ptr<TextDocument> doc, FontStyleConfig fontStyle,
							Config config ) :
	mDoc( std::move( doc ) ),
	mFontStyle( std::move( fontStyle ) ),
	mConfig( std::move( config ) ) {}

bool LineWrapping::isWrapEnabled() const {
	return mConfig.mode != LineWrapMode::NoWrap;
}

void LineWrapping::setMaxWidth( Float maxWidth, bool forceReconstructBreaks ) {
	if ( maxWidth != mMaxWidth ) {
		mMaxWidth = maxWidth;
		reconstructBreaks();
	} else if ( forceReconstructBreaks || mPendingReconstruction ) {
		reconstructBreaks();
	}
}

void LineWrapping::setFontStyle( FontStyleConfig fontStyle ) {
	if ( fontStyle != mFontStyle ) {
		mFontStyle = std::move( fontStyle );
		reconstructBreaks();
	}
}

void LineWrapping::setLineWrapMode( LineWrapMode mode ) {
	if ( mode != mConfig.mode ) {
		mConfig.mode = mode;
		reconstructBreaks();
	}
}

TextPosition LineWrapping::getDocumentLine( Int64 visibleIndex ) const {
	if ( mConfig.mode == LineWrapMode::NoWrap || mWrappedLines.empty() )
		return { visibleIndex, 0 };
	return mWrappedLines[eeclamp( visibleIndex, 0ll,
								  eemax( static_cast<Int64>( mWrappedLines.size() ) - 1, 0ll ) )];
}

Float LineWrapping::getLineOffset( Int64 docIdx ) const {
	if ( mConfig.mode == LineWrapMode::NoWrap || mWrappedLinesOffset.empty() )
		return 0;
	return mWrappedLinesOffset[eeclamp(
		docIdx, 0ll, eemax( static_cast<Int64>( mWrappedLinesOffset.size() ) - 1, 0ll ) )];
}

Int64 LineWrapping::getVisualIndexFromWrappedIndex( Int64 wrappedIndex ) const {
	Int64 idx = wrappedIndex;
	if ( mFoldedRegions.empty() )
		return idx;
	Int64 docIdx = mWrappedLines[wrappedIndex].line();
	for ( const auto& fold : mFoldedRegions ) {
		if ( fold.start().line() < docIdx ) {
			idx -=
				foldRegionVisualLength( fold.start().line(), eemin( fold.end().line(), docIdx ) );
		} else if ( fold.start().line() > docIdx ) {
			break;
		}
	}
	return idx;
}

Int64 LineWrapping::getVisualIndex( Int64 docIdx ) const {
	return getVisualIndexFromWrappedIndex( toWrappedIndex( docIdx ) );
}

Float LineWrapping::getLineYOffset( Int64 docIdx, Float lineHeight ) const {
	return getVisualIndex( docIdx ) * lineHeight;
}

void LineWrapping::setConfig( Config config ) {
	if ( config != mConfig ) {
		mConfig = std::move( config );
		reconstructBreaks();
	}
}

void LineWrapping::reconstructBreaks() {
	if ( 0 == mMaxWidth || !mDoc )
		return;

	if ( mDoc->isLoading() ) {
		mPendingReconstruction = mConfig.mode != LineWrapMode::NoWrap;
		return;
	}

	BoolScopedOp op( mUnderConstruction, true );

	mWrappedLines.clear();
	mWrappedLineToIndex.clear();
	mWrappedLinesOffset.clear();
	mLinesHidden.clear();

	if ( mConfig.mode == LineWrapMode::NoWrap )
		return;

	Int64 linesCount = mDoc->linesCount();
	mWrappedLines.reserve( linesCount );
	mWrappedLinesOffset.reserve( linesCount );
	mLinesHidden.reserve( linesCount );

	for ( auto i = 0; i < linesCount; i++ ) {
		auto lb = computeLineBreaks( *mDoc, i, mFontStyle, mMaxWidth, mConfig.mode,
									 mConfig.keepIndentation, mConfig.tabWidth );
		mWrappedLinesOffset.emplace_back( lb.paddingStart );
		mLinesHidden.emplace_back( isFolded( i ) );
		for ( const auto& col : lb.wraps )
			mWrappedLines.emplace_back( i, col );
	}

	std::optional<Int64> lastWrap;
	size_t i = 0;
	mWrappedLineToIndex.reserve( linesCount );

	for ( const auto& wl : mWrappedLines ) {
		if ( !lastWrap || *lastWrap != wl.line() ) {
			mWrappedLineToIndex.emplace_back( i );
			lastWrap = wl.line();
		}
		i++;
	}

	mPendingReconstruction = false;
}

Int64 LineWrapping::toWrappedIndex( Int64 docIdx, bool retLast ) const {
	if ( mConfig.mode == LineWrapMode::NoWrap || mWrappedLineToIndex.empty() )
		return docIdx;
	auto idx = mWrappedLineToIndex[eeclamp( docIdx, 0ll,
											static_cast<Int64>( mWrappedLineToIndex.size() - 1 ) )];
	if ( retLast ) {
		Int64 lastOfLine = mWrappedLines[idx].line();
		Int64 wrappedCount = mWrappedLines.size();
		for ( auto i = idx + 1; i < wrappedCount; i++ ) {
			if ( mWrappedLines[i].line() == lastOfLine )
				idx = i;
			else
				break;
		}
	}
	return idx;
}

bool LineWrapping::isWrappedLine( Int64 docIdx ) const {
	if ( isWrapEnabled() && mConfig.mode != LineWrapMode::NoWrap ) {
		Int64 wrappedIndex = toWrappedIndex( docIdx );
		return wrappedIndex + 1 < static_cast<Int64>( mWrappedLines.size() ) &&
			   mWrappedLines[wrappedIndex].line() == mWrappedLines[wrappedIndex + 1].line();
	}
	return false;
}

LineWrapping::VisualLine LineWrapping::getVisualLine( Int64 docIdx ) const {
	VisualLine line;
	if ( mConfig.mode == LineWrapMode::NoWrap ) {
		line.visualLines.push_back( { docIdx, 0 } );
		line.visualIndex = getVisualIndex( docIdx );
		return line;
	}
	Int64 fromIdx = toWrappedIndex( docIdx );
	Int64 toIdx = toWrappedIndex( docIdx, true );
	line.visualLines.reserve( toIdx - fromIdx + 1 );
	for ( Int64 i = fromIdx; i <= toIdx; i++ )
		line.visualLines.emplace_back( mWrappedLines[i] );
	line.visualIndex = getVisualIndexFromWrappedIndex( fromIdx );
	line.paddingStart = mWrappedLinesOffset[docIdx];
	line.visible = mLinesHidden[docIdx];
	return line;
}

LineWrapping::VisualLineInfo LineWrapping::getVisualLineInfo( const TextPosition& pos,
															  bool allowVisualLineEnd ) const {
	if ( mConfig.mode == LineWrapMode::NoWrap ) {
		LineWrapping::VisualLineInfo info;
		info.visualIndex = getVisualIndex( pos.line() );
		info.range = mDoc->getLineRange( pos.line() );
		return info;
	}
	Int64 fromIdx = toWrappedIndex( pos.line() );
	Int64 toIdx = toWrappedIndex( pos.line(), true );
	LineWrapping::VisualLineInfo info;
	for ( Int64 i = fromIdx; i < toIdx; i++ ) {
		Int64 fromCol = mWrappedLines[i].column();
		Int64 toCol = i + 1 <= toIdx
						  ? mWrappedLines[i + 1].column() - ( allowVisualLineEnd ? 0 : 1 )
						  : mDoc->line( pos.line() ).size();
		if ( pos.column() >= fromCol && pos.column() <= toCol ) {
			info.visualIndex = getVisualIndexFromWrappedIndex( i );
			info.range = { { pos.line(), fromCol }, { pos.line(), toCol } };
			return info;
		}
	}
	eeASSERT( toIdx >= 0 );
	info.visualIndex = getVisualIndexFromWrappedIndex( toIdx );
	info.range = { { pos.line(), mWrappedLines[toIdx].column() },
				   mDoc->endOfLine( { pos.line(), 0ll } ) };
	return info;
}

TextRange LineWrapping::getVisualLineRange( Int64 visualLine ) const {
	if ( mConfig.mode == LineWrapMode::NoWrap )
		return mDoc->getLineRange( visualLine );
	auto start = getDocumentLine( visualLine );
	auto end = start;
	if ( visualLine + 1 < static_cast<Int64>( mWrappedLines.size() ) &&
		 mWrappedLines[visualLine + 1].line() == start.line() ) {
		end.setColumn( mWrappedLines[visualLine + 1].column() );
	} else {
		end.setColumn( mDoc->line( start.line() ).size() );
	}
	return { start, end };
}

std::shared_ptr<TextDocument> LineWrapping::getDocument() const {
	return mDoc;
}

void LineWrapping::setDocument( const std::shared_ptr<TextDocument>& doc ) {
	if ( mDoc != doc ) {
		mDoc = doc;
		reconstructBreaks();
	}
}

bool LineWrapping::isPendingReconstruction() const {
	return mPendingReconstruction;
}

void LineWrapping::setPendingReconstruction( bool pendingReconstruction ) {
	mPendingReconstruction = pendingReconstruction;
}

void LineWrapping::clear() {
	mWrappedLines.clear();
	mWrappedLineToIndex.clear();
	mWrappedLinesOffset.clear();
	mLinesHidden.clear();
	mFoldingRegions.clear();
	mFoldedRegions.clear();
}

void LineWrapping::updateBreaks( Int64 fromLine, Int64 toLine, Int64 numLines ) {
	if ( mConfig.mode == LineWrapMode::NoWrap )
		return;
	// Get affected wrapped range
	Int64 oldIdxFrom = toWrappedIndex( fromLine, false );
	Int64 oldIdxTo = toWrappedIndex( toLine, true );

	// Remove old wrapped lines
	mWrappedLines.erase( mWrappedLines.begin() + oldIdxFrom, mWrappedLines.begin() + oldIdxTo + 1 );

	// Remove old offsets
	mWrappedLinesOffset.erase( mWrappedLinesOffset.begin() + fromLine,
							   mWrappedLinesOffset.begin() + toLine + 1 );

	// Remove old line visibility
	mLinesHidden.erase( mLinesHidden.begin() + fromLine, mLinesHidden.begin() + toLine + 1 );

	// Shift the line numbers
	if ( numLines != 0 ) {
		Int64 wrappedLines = mWrappedLines.size();
		for ( Int64 i = oldIdxFrom; i < wrappedLines; i++ )
			mWrappedLines[i].setLine( mWrappedLines[i].line() + numLines );

		shiftFoldingRegions( fromLine, numLines );
	}

	// Recompute line breaks
	auto netLines = toLine + numLines;
	auto idxOffset = oldIdxFrom;
	for ( auto i = fromLine; i <= netLines; i++ ) {
		auto lb = computeLineBreaks( *mDoc, i, mFontStyle, mMaxWidth, mConfig.mode,
									 mConfig.keepIndentation, mConfig.tabWidth );
		mWrappedLinesOffset.insert( mWrappedLinesOffset.begin() + i, lb.paddingStart );
		mLinesHidden.insert( mLinesHidden.begin() + i, isFolded( i ) );
		for ( const auto& col : lb.wraps ) {
			mWrappedLines.insert( mWrappedLines.begin() + idxOffset, { i, col } );
			idxOffset++;
		}
	}

	// Recompute wrapped line to index
	Int64 line = fromLine;
	Int64 wrappedLinesCount = mWrappedLines.size();
	mWrappedLineToIndex.resize( mDoc->linesCount() );
	for ( Int64 wrappedIdx = oldIdxFrom; wrappedIdx < wrappedLinesCount; wrappedIdx++ ) {
		if ( mWrappedLines[wrappedIdx].column() == 0 ) {
			mWrappedLineToIndex[line] = wrappedIdx;
			line++;
		}
	}
	mWrappedLineToIndex.resize( mDoc->linesCount() );

	recalculateHiddenLines();

#ifdef EE_DEBUG
	if ( mConfig.keepIndentation ) {
		auto wrappedOffset = mWrappedLinesOffset;

		for ( auto i = fromLine; i <= toLine; i++ ) {
			mWrappedLinesOffset[i] =
				computeOffsets( mDoc->line( i ).getText(), mFontStyle, mConfig.tabWidth );
		}

		eeASSERT( wrappedOffset == mWrappedLinesOffset );
	}

	auto wrappedLines = mWrappedLines;
	auto wrappedLinesToIndex = mWrappedLineToIndex;
	auto wrappedOffset = mWrappedLinesOffset;

	reconstructBreaks();

	eeASSERT( wrappedLines == mWrappedLines );
	eeASSERT( wrappedLinesToIndex == mWrappedLineToIndex );
	eeASSERT( wrappedOffset == mWrappedLinesOffset );
#endif
}

size_t LineWrapping::getTotalLines() const {
	return mConfig.mode == LineWrapMode::NoWrap ? mDoc->linesCount() - mHiddenLinesCount
												: mWrappedLines.size() - mHiddenVisualLinesCount;
}

std::pair<Uint64, Uint64> LineWrapping::getVisibleLineRange( Int64 startVisualLine,
															 Int64 viewLineCount,
															 bool visualIndexes ) const {
	if ( mFoldedRegions.empty() ) {
		if ( isWrapEnabled() && !visualIndexes ) {
			return std::make_pair<Uint64, Uint64>(
				(Uint64)getDocumentLine( startVisualLine ).line(),
				(Uint64)getDocumentLine( startVisualLine + viewLineCount ).line() );
		}
		return std::make_pair<Uint64, Uint64>(
			(Uint64)startVisualLine,
			eemin( (Uint64)( startVisualLine + viewLineCount ),
				   (Uint64)( visualIndexes ? getTotalLines() - 1 : mDoc->linesCount() - 1 ) ) );
	}
	if ( isWrapEnabled() && visualIndexes ) {
		Int64 startDocIdx = getDocumentLine( startVisualLine ).line();
		Int64 viewLineLeft = viewLineCount;
		Int64 linesCount = mWrappedLines.size();
		for ( Int64 i = startDocIdx + 1; i < linesCount; i++ ) {
			if ( isLineHidden( mWrappedLines[i].line() ) )
				continue;
			viewLineLeft--;
			if ( viewLineLeft == 0 )
				return { startDocIdx, getDocumentLine( i ).line() };
		}
		return { startDocIdx, mWrappedLines[mWrappedLines.size() - 1].line() };
	}
	Int64 startDocIdx = startVisualLine;
	Int64 viewLineLeft = viewLineCount;
	Int64 linesCount = mDoc->linesCount();
	for ( Int64 i = startDocIdx + 1; i < linesCount; i++ ) {
		if ( isLineHidden( startDocIdx ) )
			continue;
		viewLineLeft--;
		if ( viewLineLeft == 0 )
			return { startDocIdx, i };
	}
	return { startDocIdx, linesCount - 1 };
}

bool LineWrapping::isNextLineHidden( Int64 docIdx ) const {
	return !mLinesHidden.empty() && docIdx >= 0 &&
				   docIdx + 1 < static_cast<Int64>( mLinesHidden.size() )
			   ? mLinesHidden[docIdx + 1]
			   : false;
}

void LineWrapping::addFoldRegion( TextRange region ) {
	region.normalize();
	mFoldingRegions[region.start().line()] = std::move( region );
}

bool LineWrapping::isFoldingRegionInLine( Int64 docIdx ) {
	auto foldRegionIt = mFoldingRegions.find( docIdx );
	return foldRegionIt != mFoldingRegions.end();
}

void LineWrapping::foldRegion( Int64 foldDocIdx ) {
	auto foldRegionIt = mFoldingRegions.find( foldDocIdx );
	if ( foldRegionIt == mFoldingRegions.end() )
		return;
	Int64 toDocIdx = foldRegionIt->second.end().line();
	mHiddenLinesCount += foldRegionIt->second.height();
	mHiddenVisualLinesCount += foldRegionVisualLength( foldDocIdx, toDocIdx );
	changeVisibility( foldDocIdx, toDocIdx, true );
	mFoldedRegions.push_back( foldRegionIt->second );
	std::sort( mFoldedRegions.begin(), mFoldedRegions.end() );
}

void LineWrapping::unfoldRegion( Int64 foldDocIdx ) {
	auto foldRegionIt = mFoldingRegions.find( foldDocIdx );
	if ( foldRegionIt == mFoldingRegions.end() )
		return;
	Int64 toDocIdx = foldRegionIt->second.end().line();
	mHiddenLinesCount -= foldRegionIt->second.height();
	mHiddenVisualLinesCount -= foldRegionVisualLength( foldDocIdx, toDocIdx );
	changeVisibility( foldDocIdx, toDocIdx, true );
	removeFoldedRegion( foldRegionIt->second );
}

Int64 LineWrapping::foldRegionVisualLength( Int64 fromDocIdx, Int64 toDocIdx ) const {
	Int64 startIdx = toWrappedIndex( fromDocIdx );
	Int64 endIdx = toWrappedIndex( toDocIdx, true );
	return endIdx - startIdx + 1;
}

Int64 LineWrapping::foldRegionVisualLength( const TextRange& fold ) const {
	return foldRegionVisualLength( fold.start().line(), fold.end().line() );
}

void LineWrapping::changeVisibility( Int64 fromDocIdx, Int64 toDocIdx, bool visible ) {
	for ( Int64 i = fromDocIdx; i <= toDocIdx; i++ )
		mLinesHidden[i] = !visible;
}

void LineWrapping::removeFoldedRegion( const TextRange& region ) {
	auto found = std::find( mFoldedRegions.begin(), mFoldedRegions.end(), region );
	if ( found != mFoldedRegions.end() )
		mFoldedRegions.erase( found );
}

Int64 LineWrapping::getHiddenLinesCount() const {
	return mHiddenLinesCount;
}

bool LineWrapping::isFolded( Int64 docIdx ) const {
	return std::any_of(
		mFoldedRegions.begin(), mFoldedRegions.end(),
		[docIdx]( const TextRange& region ) { return region.containsLine( docIdx ); } );
}

bool LineWrapping::isLineHidden( Int64 docIdx ) const {
	return !mLinesHidden.empty() && docIdx >= 0 &&
				   docIdx < static_cast<Int64>( mLinesHidden.size() )
			   ? mLinesHidden[docIdx]
			   : false;
}

void LineWrapping::shiftFoldingRegions( Int64 fromLine, Int64 numLines ) {
	for ( auto& [_, region] : mFoldingRegions ) {
		if ( region.start().line() >= fromLine ) {
			region.start().setLine( region.start().line() + numLines );
			region.end().setLine( region.end().line() + numLines );
		}
	}
	for ( auto& region : mFoldedRegions ) {
		if ( region.start().line() >= fromLine ) {
			region.start().setLine( region.start().line() + numLines );
			region.end().setLine( region.end().line() + numLines );
		}
	}
}

void LineWrapping::recalculateHiddenLines() {
	mHiddenLinesCount = 0;
	mHiddenVisualLinesCount = 0;
	for ( auto& region : mFoldedRegions ) {
		mHiddenLinesCount += region.height();
		mHiddenVisualLinesCount +=
			foldRegionVisualLength( region.start().line(), region.end().line() );
	}
}

}} // namespace EE::UI
