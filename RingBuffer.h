#define DEFAULT_SIZE 5 // デフォルトのバッファサイズ

class RingBuffer
{
  private:
    int* buffer; // バッファ（動的配列）
    int size;    // バッファサイズ
    int front;   // 先頭
    int rear;    // 末尾（次のデータを追加する位置）
    int count;   // 現在のデータ数
    int lastData;// 最後に取得したデータを記録

  public:
    RingBuffer(int a_size = DEFAULT_SIZE);
    ~RingBuffer();
    void push(int data);
    bool pop(int& data);
    int getLastData() const;
    int getFromLast(int n) const;
};
