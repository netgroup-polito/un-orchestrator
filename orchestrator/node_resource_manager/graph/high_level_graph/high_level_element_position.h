#ifndef NODE_ORCHESTRATOR_POSITION_H
#define NODE_ORCHESTRATOR_POSITION_H
#include "../../../utils/constants.h"

#include <json_spirit/json_spirit.h>
#include <json_spirit/value.h>
#include <json_spirit/writer.h>

using namespace json_spirit;

namespace highlevel {

    class Position {
        int x, y;
    public:
        Position();

        int getX();

        int getY();

        void setX(int x);

        void setY(int y);

        Object toJSON();
    };
}

#endif //NODE_ORCHESTRATOR_POSITION_H
