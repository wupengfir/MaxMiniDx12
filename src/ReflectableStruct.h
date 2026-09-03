#pragma once
template<typename T> 
struct VarBase
{
public:
	size_t offset;
};

template<typename T> 
struct Var {};

template<>
struct Var<int>
{

};

struct Rstruct
{
	size_t size;
};


