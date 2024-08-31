#ifndef __NONCOPYABLE_H__
#define __NONCOPYABLE_H__

/*
禁止拷贝操作的基类，设置为protect权限的成员函数可以让派生类继承，派生类对象可以正常的构造和析构
noncopyable被继承以后，派生类对象可以正常的构造和析构，但是派生类对象无法进行拷贝构造和赋值操作
*/

// TODO: noncopyable 的学习
class noncopyable
{
public:
    noncopyable(const noncopyable&) = delete;
    noncopyable& operator=(const noncopyable&) = delete;

protected:
    noncopyable() = default;
    ~noncopyable() = default;
};

#endif // __NONCOPYABLE_H__