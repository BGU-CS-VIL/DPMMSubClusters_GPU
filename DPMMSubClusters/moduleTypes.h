#pragma once
#include <vector>

typedef int BaseType;
typedef BaseType IterationIndexType;
typedef BaseType ClusterIndexType;
typedef BaseType DimensionsType;
typedef BaseType PointType;
typedef BaseType LabelType;
typedef std::vector<LabelType> LabelsType;

//class LabelsType
//{
//public:
//	LabelsType()
//	{
//		data = NULL;
//		data_size = 0;
//		allocate_Size = 0;
//	}
//
//	LabelsType(LabelType size)
//	{
//		data = new LabelType[size];
//		data_size = size;
//		allocate_Size = size;
//	}
//
//	~LabelsType()
//	{
//		if (data != NULL)
//		{
//			delete[]data;
//		}
//	}
//
//	void reserve(int allocateSize)
//	{
//		if (data != NULL)
//		{
//			delete[]data;
//		}
//		data = new LabelType[allocateSize];
//		allocate_Size = allocateSize;
//	}
//
//	void resize(int a, LabelType b = 0)
//	{
//		if (a > allocate_Size)
//		{
//			//avoid it
//			LabelType *newData = new LabelType[a];
//
//			LabelType i = 0;
//			if (data != NULL)
//			{
//				for (; i < data_size && i < a; i++)
//				{
//					newData[i] = data[i];
//				}
//				delete[]data;
//			}
//
//			for (; i < a; i++)
//			{
//				newData[i] = b;
//			}
//			data = newData;
//			allocate_Size = a;
//		}
//		
//		data_size = a;
//	}
//
//	void push_back(LabelType a)
//	{
//		//avoid it
//		resize(data_size + 1, a);
//		data[data_size - 1] = a;
//	}
//
//	LabelType size() const
//	{
//		return data_size;
//	}
//
//	LabelType& operator[](LabelType index) const
//	{
//		if (index >= data_size) {
//			printf("Array index out of bound, exiting\n");
//			exit(0);
//		}
//		return data[index];
//	}
//
//	LabelType* operator+=(LabelsType &toAdd)
//	{
//		LabelType oldSize = data_size;
//		resize(data_size + toAdd.size());
//		for (LabelType i = 0; i < toAdd.size(); i++)
//		{
//			data[oldSize + i] = toAdd[i];
//		}
//		return data;
//	}
//
//	void fill(LabelType start)
//	{
//		for (LabelType i = 0; i < data_size; i++)
//		{
//			data[i] = start + i;
//		}
//	}
//
//	LabelType *data;
//private:
//	LabelType data_size;
//	LabelType allocate_Size;
//};
