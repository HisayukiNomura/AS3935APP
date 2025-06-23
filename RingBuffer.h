// RingBufferT.h
/**
 * @file RingBuffer.h
 * @brief 汎用リングバッファ（テンプレート）クラス宣言
 * @details
 * 任意型Tのデータを固定長で循環管理できるリングバッファ。
 * push/pop/最新値取得/逆順アクセス/要素数取得などをサポート。
 */
#pragma once

/**
 * @brief 汎用リングバッファ（循環バッファ）テンプレートクラス
 * @details
 * pushでデータを追加し、popでFIFO取得。最新値や逆順アクセスも可能。
 * バッファサイズを超えると古いデータから上書きされる。
 * @tparam T バッファに格納する型
 */
template <typename T>
class RingBufferT
{
  private:
	T* buffer;      ///< バッファ本体
	int size;       ///< バッファサイズ
	int front;      ///< 先頭インデックス
	int rear;       ///< 末尾インデックス
	int count;      ///< 現在の要素数
	T lastData;     ///< 最後にpushされたデータ

  public:
	/**
	 * @brief コンストラクタ
	 * @param a_size バッファサイズ
	 */
	RingBufferT(int a_size)
	{
		size = a_size;
		buffer = new T[size];
		front = 0;
		rear = 0;
		count = 0;
		lastData = 0;
	}
	/**
	 * @brief デストラクタ
	 */
	~RingBufferT()
	{
		delete[] buffer;
	}
	/**
	 * @brief データをバッファに追加
	 * @details
	 * バッファが満杯の場合は最古データを上書き。
	 * @param data 追加するデータ
	 */
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
	/**
	 * @brief 先頭データを取り出す（FIFO）
	 * @param data 取り出したデータ格納先
	 * @return データが存在した場合true、空ならfalse
	 */
	bool pop(T& data)
	{
		if (count == 0) return false;
		data = buffer[front];
		front = (front + 1) % size;
		count--;
		return true;
	}
	/**
	 * @brief 最後にpushされたデータを取得
	 * @return 最新データ
	 */
	T getLastData() const
	{
		return lastData;
	}
	/**
	 * @brief 末尾からn番目のデータを取得
	 * @details
	 * n=0で最新、n=1で1つ前、...。範囲外は0を返す。
	 * @param n 末尾からのオフセット
	 * @return 指定位置のデータ
	 */
	T getFromLast(int n) const
	{
		if (n < 0 || n >= count) return 0;
		int idx = (rear - 1 - n + size) % size;
		return buffer[idx];
	}
	/**
	 * @brief 現在の要素数を取得
	 * @return バッファ内の要素数
	 */
	int getCount() const { return count; }
};