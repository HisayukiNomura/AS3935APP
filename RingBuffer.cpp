#include "RingBuffer.h"

RingBuffer::RingBuffer(int a_size)
{
    size = a_size;
    buffer = new int[size];
    front = 0;
    rear = 0;
    count = 0;
    lastData = 0;
}

RingBuffer::~RingBuffer()
{
    delete[] buffer;
}

void RingBuffer::push(int data)
{
    if (count == size) {
        front = (front + 1) % size; // 古いデータを削除（先頭を更新）
    } else {
        count++;
    }
	lastData = data; // 取得したデータを記録
	buffer[rear] = data;
    rear = (rear + 1) % size;
}

bool RingBuffer::pop(int& data)
{
    if (count == 0) return false; // バッファが空
    data = buffer[front];
    front = (front + 1) % size;
    count--;

    return true;
}

int RingBuffer::getLastData() const
{
    return lastData;
}

int RingBuffer::getFromLast(int n) const
{
    if (n < 0 || n >= count) return 0; // 範囲外
    int idx = (rear - 1 - n + size) % size;
    return buffer[idx];
}
