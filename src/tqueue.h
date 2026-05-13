#if !defined(TQUEUE_H)
#define TQUEUE_H

#include <array>
#include <chrono>
#include <condition_variable>
#include <mutex>

template<typename T, size_t N>
class TQueue {
public:
	TQueue() : m_buffer{}
	         , m_cond{}
	         , m_mutex{}
	         , m_head{}
	         , m_tail{} {};
	~TQueue() noexcept {};

	TQueue(const TQueue& other) = delete;
	TQueue operator=(const TQueue& rhs) = delete;

	TQueue(const TQueue&& other) = delete;
	TQueue operator=(const TQueue&& rhs) = delete;

	bool isEmpty() const noexcept;
	size_t count() const noexcept;

	void push(const T& x);
	void push(T&& x);
	T pop();

	typedef void (*TransformerFn)(size_t n, T* ts);
	void realign();
	void transform(TransformerFn fn);

private:
	std::array<T, N> m_buffer;
	std::condition_variable m_cond;
	std::mutex m_mutex;

	size_t m_head = 0;
	size_t m_tail = 0;
};

template<typename T, size_t N>
bool TQueue<T, N>::isEmpty() const noexcept {
	return (m_head % N) == (m_tail % N);
};

template<typename T, size_t N>
size_t TQueue<T, N>::count() const noexcept {
	return (N + m_head - m_tail) % N;
};

template<typename T, size_t N>
void TQueue<T, N>::push(const T& x) {
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		m_buffer[m_head] = std::move(x);
		m_head = (m_head + 1) % N;
	};

	m_cond.notify_one();
};

template<typename T, size_t N>
void TQueue<T, N>::push(T&& x) {
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		m_buffer[m_head] = std::move(x);
		m_head = (m_head + 1) % N;
	};

	m_cond.notify_one();
};

template<typename T, size_t N>
T TQueue<T, N>::pop() {
	std::unique_lock<std::mutex> lock(m_mutex);
	m_cond.wait(lock, [this] {return !this->isEmpty(); });

	T x = m_buffer[m_tail];
	m_tail = (m_tail + 1) % N;

	return x;
};

template<typename T, size_t N>
void TQueue<T, N>::realign() {
	if (m_tail < m_head) {
		std::lock_guard<std::mutex> lock(m_mutex);
		const size_t n = count();
		std::reverse(m_array.begin(), m_array.begin() + m_head);
		std::reverse(m_array.begin(), m_array.begin() + n);
		m_head = n;
		m_tail = 0;
	} else {
		std::lock_guard<std::mutex> lock(m_mutex);
		const size_t n = count();
		std::reverse(m_array.begin() + m_head, m_array.end());
		std::reverse(m_array.begin(), m_array.begin() + m_head);
		std::reverse(m_array.begin(), m_array.end());
		m_head = n;
		m_tail = 0;
	};

	m_cond.notify_one();
};

template<typename T, size_t N>
T* TQueue<T, N>::data() {
	return m_array.data();
};

#endif // TQUEUE_H