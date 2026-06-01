#pragma once

class BlockQuery {
public:
    virtual ~BlockQuery() = default;
    virtual bool hasBlockGlobal(int wx, int wy, int wz) const = 0;
};
