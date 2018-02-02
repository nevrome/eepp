#include <eepp/scene/nodemessage.hpp>
#include <eepp/scene/node.hpp>

namespace EE { namespace Scene {

NodeMessage::NodeMessage( Node * node, const Uint32& Msg, const Uint32& Flags ) :
	mCtrl( node ),
	mMsg( Msg ),
	mFlags( Flags )
{
}

NodeMessage::~NodeMessage()
{
}

Node * NodeMessage::getSender() const {
	return mCtrl;
}

const Uint32& NodeMessage::getMsg() const {
	return mMsg;
}

const Uint32& NodeMessage::getFlags() const {
	return mFlags;
}

}}
