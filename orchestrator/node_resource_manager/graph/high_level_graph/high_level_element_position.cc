#include "high_level_element_position.h"

namespace highlevel {

    Position::Position() {
        x = 0;
        y = 0;
    }

    int Position::getX() {
        return x;
    }

    int Position::getY() {
        return y;
    }

    void Position::setX(int x) {
        this->x = x;
    }

    void Position::setY(int y) {
        this->y = y;
    }

    Object Position::toJSON() {
        Object pos;
        pos[X_POSITION] = x;
        pos[Y_POSITION] = y;
        return pos;
    }

}