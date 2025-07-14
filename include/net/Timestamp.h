#pragma once

#include <iostream>

class Timestamp
{
public:
    Timestamp();

    int64_t operator-(const Timestamp &other)
    {
        return other.microSecondsSinceEpoch_ - microSecondsSinceEpoch_;
    }

    explicit Timestamp(int64_t microSecondsSinceEpoch);

    static Timestamp now();
    std::string toString() const;

private:
    int64_t microSecondsSinceEpoch_;
};