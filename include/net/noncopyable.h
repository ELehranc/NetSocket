#pragma once

/* 
    noncopyabel 被继承以后，派生类可以正常地构造和析构，但是派生类无法被拷贝构造和赋值
*/
class noncopyable
{

public:
    noncopyable(const noncopyable &) = delete;
    noncopyable &operator=(const noncopyable &) = delete;

protected:
    noncopyable() = default;
    ~noncopyable() = default;
};
