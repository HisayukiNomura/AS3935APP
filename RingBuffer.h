// RingBufferT.h
#pragma once

template <typename T>
class RingBufferT
{
  private:
	T* buffer;
	int size;
	int front;
	int rear;
	int count;
	T lastData;

  public:
	RingBufferT(int a_size)
	{
		size = a_size;
		buffer = new T[size];
		front = 0;
		rear = 0;
		count = 0;
		lastData = 0;
	}
	~RingBufferT()
	{
		delete[] buffer;
	}
	void push(T data)
	{
		if (count == size) {
			front = (front + 1) % size;
		} else {
			count++;
		}
		lastData = data;
		buffer[rear] = data;
		rear = (rear + 1) % size;
	}
	bool pop(T& data)
	{
		if (count == 0) return false;
		data = buffer[front];
		front = (front + 1) % size;
		count--;
		return true;
	}
	T getLastData() const
	{
		return lastData;
	}
	T getFromLast(int n) const
	{
		if (n < 0 || n >= count) return 0;
		int idx = (rear - 1 - n + size) % size;
		return buffer[idx];
	}
	int getCount() const { return count; }
};