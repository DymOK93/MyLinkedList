/*********************
Dmitry Bolshakov, 2020
v2.3
*********************/
#pragma once
#ifndef MyLinkedList_H
#define MyLinkedList_H
#define PRED_TO_BOOL(pred, x, y) static_cast<bool>(pred(x, y))
#undef RING_LIST				
#include "MyListAllocator.h"					//Управление памятью
#include <xstddef>								//Для сравнения элементов
#include <utility>								//Для std::forward
#include <memory>								//Для механизма общих данных
#include <functional>							//Для std::function
#define CONTAINER_VERIFY(cond, what) _STL_VERIFY(cond, what)

template <class T>
class MyLinkedList {
public:
	using value_type = T;
	using size_type = size_t;
	using difference_type = std::ptrdiff_t;
	using reference = value_type&;
	using const_reference = const value_type&;
	using pointer = value_type*;
	using const_pointer = const value_type*;
	class iterator;
	class const_iterator;
	using reverse_iterator = std::reverse_iterator<iterator>;
	using const_reverse_iterator = std::reverse_iterator<const_iterator>;
private:
	struct BaseNode;
	struct Node;
	class ChainBuilder;
	template<class Predicate> class BSTSortHelper;
	using Allocator = MyListAllocator<Node>;
	using SharedAllocator = std::shared_ptr<Allocator>;
private:
	SharedAllocator alc;
	Node *my_end;							//|my_end(no data)|<---|(data)|<--->...<--->|(data)|--->|my_end(no data)|
	size_t my_size;
public:
	inline MyLinkedList() : alc{ nullptr }, my_end{ nullptr }, my_size{ 0 } {}
	explicit MyLinkedList(size_t count);
	inline MyLinkedList(const std::initializer_list<T>& init) : MyLinkedList() { detach_helper();  copy_container(init); }
	inline MyLinkedList(const MyLinkedList& o) noexcept : my_end{ o.my_end }, my_size{ o.my_size }, alc{ o.alc } {}
	inline MyLinkedList(MyLinkedList&& o) noexcept : MyLinkedList() { move_list(std::move(o)); }
	MyLinkedList<T>& operator=(const MyLinkedList<T>& o);
	MyLinkedList<T>& operator=(MyLinkedList<T>&& o) noexcept;
	inline ~MyLinkedList() noexcept { if (!is_shared()) clear(); }
public:
	inline bool is_shared() const noexcept { return alc.use_count() != 1; }		
	inline bool is_shared_with(const MyLinkedList<T>& o) const noexcept { return alc == o.alc;}
	inline size_t shared_data_use_count() const noexcept { return alc.use_count(); }
	inline size_t size() const noexcept { return my_size; }
	inline bool empty() const noexcept { return my_size == 0; }
	inline bool isEmpty() const noexcept { return my_size == 0; }
private:
	void detach_helper(Node** first_target = nullptr, Node** second_target = nullptr);

	void detach_copy_helper(ChainBuilder& cb, Node* begin, Node* end, std::pair<Node*, Node*>& targets);

	template<class ...Types> 
	inline Node* create_node(Node* prev, Node* next, Types&&... Args) { return new(alc->allocate()) Node(prev, next, std::forward<Types>(Args)...); }

	inline Node* create_base_node()
	{ Node* new_node{ reinterpret_cast<Node*>(new (alc->allocate()) BaseNode()) }; new_node->n = new_node; new_node->p = new_node; return new_node;}

	inline void destroy_node(Node* destroyed_node) noexcept {  destroyed_node->~Node(); alc->deallocate(destroyed_node); }

	template<class ...Types>
	inline void emplace_helper(Node* prev, Node* next, Types&&... Args)
	{ Node* new_node{ create_node(prev, next, std::forward<Types>(Args)...) }; prev->n = new_node; next->p = new_node; }

	void displace_helper(Node* target) noexcept { target->p->n = target->n; target->n->p = target->p; destroy_node(target); }

	template<class Predicate> size_t remove_helper(Predicate pred);

	inline size_t delete_helper(Node* first, Node* last) noexcept 
	{ size_t count{ 1 };  Node* target; while (last != first) { target = last; last = last->p; destroy_node(target); ++count;  } destroy_node(first); return count; }

	void move_list(MyLinkedList<T>&&) noexcept;

	template <class Container> 
	void copy_container(const Container& cont);									//Копирует переданный в качестве аргумента контейнер в список
public:
	template<class ...Types> inline void emplace_back(Types&&... Args) 
	{ if (is_shared()) detach_helper(); emplace_helper(my_end->p, my_end, std::forward<Types>(Args)...); ++my_size;}
	template<class ...Types> inline void emplace_front(Types&&... Args) 
	{ if (is_shared()) detach_helper(); emplace_helper(my_end, my_end->n, std::forward<Types>(Args)...); ++my_size; }

	inline void push_back(T&& val) { emplace_back(std::move(val)); }
	inline void push_front(T&& val) { emplace_front(std::move(val)); }
	inline void push_back(const T& val) { emplace_back(val); }
	inline void push_front(const T& val) { emplace_front(val); }
	inline void append(T&& val) { emplace_back(std::move(val)); }
	inline void prepend(T&& val) { emplace_front(std::move(val)); }
	inline void append(const T& val) { emplace_back(val); }
	inline void prepend(const T& val) { emplace_front(val); }

	void pop_back() noexcept;
	void pop_front() noexcept;
	inline void removeFirst() noexcept { pop_front(); }
	inline void removeLast() noexcept { pop_back(); }

	template<class ...Types> iterator emplace(iterator before, Types&&... Args);
	inline iterator insert(iterator before, T&& val) { return emplace(before, std::move(val)); }
	inline iterator insert(iterator before, const T& val) { return emplace(before, val); }
	template<class InputIt> iterator insert(iterator before, InputIt first, InputIt last);

	iterator erase(iterator) noexcept;
	iterator erase(iterator first, iterator last) noexcept;
public:		
	inline T& front() noexcept { CONTAINER_VERIFY(!(empty()), "Empty list"); return *begin(); }	//Некоторые методы имеют разные названия, но одинаковый функционал
	inline T& first() noexcept { return front(); }												//Так сделано для обеспечения совместимости 	
	inline T& back() noexcept { CONTAINER_VERIFY(!(empty()), "Empty list"); return *(--end()); }//как с STL-style, так и с и Qt-style обёртками для контейнеров	
	inline T& last() noexcept { return back(); }																								
	
	inline const T& first() const noexcept { CONTAINER_VERIFY(!(empty()), "Empty list"); return *begin(); }
	inline const T& front() const noexcept { return first(); }
	inline const T& last() const noexcept { CONTAINER_VERIFY(!(empty()), "Empty list"); return *(--end()); }
	inline const T& back() const noexcept { return last(); }

	inline T takeFirst() noexcept				//Объект выгоднее переместить, чем скопировать. Но, во-первых, требуется предварительно открепиться от общего хранилища, 
	{ CONTAINER_VERIFY(!(empty()), "Empty list"); if (is_shared()) detach_helper(); T val{ std::move_if_noexcept(my_end->n->val) }; pop_front();  return val; }
	inline T takeLast() noexcept				//во-вторых, не нарушить целостность контейнера при выбросе исключения
	{ CONTAINER_VERIFY(!(empty()), "Empty list"); if (is_shared()) detach_helper(); T val{ std::move_if_noexcept(my_end->p->val) }; pop_back(); return val; }
public:
	bool contains(const T&) const noexcept;
	size_t count(const T& val) const noexcept;
	inline bool startsWith(const T& val) const noexcept { if (!empty()) return first() == val; return false; }
	inline bool endsWith(const T& val) const noexcept { if (!empty()) return last() == val; return false; }
	inline bool removeOne(const T& val) noexcept { iterator target{ find(val) }; if (target != end()) { erase(target); return true; } return false;}
	inline size_t remove(const T& val) noexcept { return removeAll(val); }
	template<class Predicate> inline size_t remove_if(Predicate pred) { return remove_helper(pred); }
	size_t removeAll(const T& val) noexcept { return remove_helper([&val](auto&& node_val) {return node_val == val; }); }
public:
	inline iterator find(const T& val) { return find(val, begin()); }
	inline iterator find(const T& val, iterator it)	{ for (; it != end() && *it != val; ++it); return it; }

	inline iterator findFromEnd(const T& val) { return findFromEnd(val, end()); }
																			//Проверка if(!empty()) - во избежание разыменования итератора end в пустом контейнере
	inline iterator findFromEnd(const T& val, iterator it)	{ if (!empty()) do { if (*(--it) == val) return it; }while (it != begin()); return end(); }
public:
	inline void clear() noexcept { if (!empty()) { delete_helper(my_end->n, my_end->p);  alc->clear();  my_end->p = my_end; my_end->n = my_end; my_size = 0; } }
	inline void reserve(size_t size) { if (size > my_size) alc->reserve(size-my_size); }

	void swap(MyLinkedList<T>& o) noexcept;

	inline void sort() { sort(std::less<>()); }

	template<class Predicate> inline void sort(Predicate comparator) 
	{ if (is_shared()) detach_helper(); merge_sort(my_end->n, my_end, my_size, comparator); }

	inline void sort(MyLinkedList<T>::iterator begin, MyLinkedList<T>::iterator end) { sort(begin, end, std::less<>()); }

	template<class Predicate> void sort(MyLinkedList<T>::iterator begin, MyLinkedList<T>::iterator end, Predicate comparator);																//Статические функции класса: сортировка, свап элементов
	
	static void swap(MyLinkedList<T>::iterator, MyLinkedList<T>::iterator) noexcept;
public:																		//Функции для инициализации итераторов
																			//Если список пуст, const_iterator будет содержать nullptr или my_end
	inline iterator begin() { if (is_shared()) detach_helper(); return iterator(my_end->n, this); }
	inline reverse_iterator rbegin() noexcept { return std::make_reverse_iterator<iterator>(begin()); }
	inline const_iterator begin() const noexcept { return const_iterator(my_end ? my_end->n : my_end, this); }
	inline const_iterator cbegin() const noexcept { return begin(); }
	inline const_reverse_iterator rbegin() const noexcept { return std::make_reverse_iterator<const_iterator>(begin()); }
	inline const_reverse_iterator crbegin() const noexcept { return std::make_reverse_iterator<const_iterator>(cbegin()); }
	inline const_iterator constBegin() const noexcept { return begin(); }

	inline iterator end() { if (is_shared()) detach_helper(); return iterator(my_end, this); }
	inline reverse_iterator rend() noexcept { return std::make_reverse_iterator<iterator>(end()); }
	inline const_iterator end() const noexcept { return const_iterator(my_end, this); }
	inline const_iterator cend() const noexcept { return end(); }
	inline const_reverse_iterator rend() const noexcept { return std::make_reverse_iterator<const_iterator>(end()); }
	inline const_reverse_iterator crend() const noexcept { return std::make_reverse_iterator<const_iterator>(cend()); }
	inline const_iterator constEnd() const noexcept { return end(); }

public:																		//Перегруженные операторы
	inline MyLinkedList<T>& operator+=(const T& val) { emplace_back(val); return *this; }
	inline MyLinkedList<T>& operator+=(T&& val) { emplace_back(std::move(val)); return *this; }
	inline MyLinkedList<T>& operator<<(const T& val) { emplace_back(val); return *this; }
	inline MyLinkedList<T>& operator<<(T&& val) { emplace_back(std::move(val)); return *this; }
	MyLinkedList<T>& operator+=(const MyLinkedList<T>&);
	inline MyLinkedList<T> operator+(const MyLinkedList<T>& o) { MyLinkedList<T> result{ *this }; result += o; return result; }
	bool operator==(const MyLinkedList<T>& o) const;						//Не noexcept, т.к. operator== для объектов класса T может быть не noexcept
	inline bool operator!=(const MyLinkedList<T>& o) const { return !(*this == o); }

private:
	template<class Predicate>
	static void tree_sort(MyLinkedList<T>::iterator& begin, MyLinkedList<T>::iterator& end, Predicate pred);

	template<class Predicate> static Node* merge_sort(Node* first, Node* last, size_t size, Predicate pred);
	static Node* merge_helper(Node* first, Node* last);

	template <class ContainerIterator>										//Копирует данные из отрезка [begin; end-1] в промежуточный контейнер ChainBuilder
	inline size_t copy_helper(ChainBuilder& cb, const ContainerIterator& begin, const ContainerIterator& end) 
	{ size_t count{ 0 };  for (auto it = begin; it != end; ++it, ++count) cb.attach(create_node(nullptr, nullptr, *it)); return count; }
public:																		//Итераторы
	class iterator {
	public:
		using iterator_category = std::bidirectional_iterator_tag;
		using value_type = MyLinkedList<T>::value_type;
		using difference_type = ptrdiff_t;
		using pointer = MyLinkedList<T>::pointer;
		using reference = MyLinkedList<T>::reference;
	private:
		friend class MyLinkedList<T>;
		Node* my_node;
		MyLinkedList<T>* my_cont;
	public:
		inline iterator() noexcept : my_node{ nullptr }, my_cont{ nullptr } {}
		inline iterator(Node* node, MyLinkedList<T>* cont) noexcept : my_node{ node }, my_cont{ cont } {}
		inline iterator(const iterator& o) noexcept = default;
		inline iterator& operator=(const iterator&) noexcept = default;
		inline ~iterator() noexcept = default;
	public:
		iterator& operator++() noexcept;
		iterator& operator--() noexcept;
		iterator operator++(int) noexcept;
		iterator operator--(int) noexcept;
		inline iterator& operator+=(int offset) noexcept 
		{ if (offset > 0) while (offset > 0) { ++(*this); --offset; } else while (offset < 0) { --(*this); ++offset; } return *this; }
		inline iterator& operator-=(int offset) noexcept { return *this += (-offset); }
		inline iterator operator+(int offset) noexcept { iterator it{ *this };  return it += offset; }
		inline iterator operator-(int offset) noexcept { iterator it{ *this }; return it += (-offset); }
		inline friend iterator operator+(int offset, iterator it) noexcept { return it + offset; }
		inline bool operator==(const iterator& o) const noexcept { return (my_node == o.my_node); }
		inline bool operator!=(const iterator& o) const noexcept { return !(my_node == o.my_node); }
		inline T& operator*() const noexcept { CONTAINER_VERIFY(my_node != my_cont->cend().my_node, "Can't dereference end iterator"); return my_node->val; }
		inline T* operator->() const noexcept 
		{ CONTAINER_VERIFY(my_node != my_cont->cend().my_node, "Can't dereference end list iterator"); return std::addressof(my_node->val); }
	};

	class const_iterator {
	public:
		using iterator_category = std::bidirectional_iterator_tag;
		using value_type = MyLinkedList<T>::value_type;
		using difference_type = ptrdiff_t;
		using pointer = MyLinkedList<T>::pointer;
		using reference = MyLinkedList<T>::reference;
	private:
		friend class iterator;
		friend class MyLinkedList<T>;
		Node* my_node;
		const MyLinkedList<T>* const my_cont;
	public:
		inline const_iterator() noexcept  : my_node{ nullptr }, my_cont{ nullptr } {}
		inline const_iterator(Node* node, const MyLinkedList<T>* const cont) noexcept : my_node{ node }, my_cont{ cont } {}
		inline const_iterator(const const_iterator& o) noexcept = default;
		inline const_iterator& operator=(const const_iterator&) noexcept = default;
		inline ~const_iterator() noexcept = default;
	public:
		const_iterator& operator++();
		const_iterator& operator--();
		const_iterator operator++(int);
		const_iterator operator--(int);
		inline const_iterator& operator+=(int offset) noexcept 
		{if (offset > 0)while (offset > 0){ ++(*this); --offset; }else while (offset < 0) { --(*this); ++offset; }return *this;}
		inline const_iterator& operator-=(int offset) noexcept { return *this += (-offset); }
		inline const_iterator operator+(int offset) noexcept { iterator it{ *this };  return it += offset; }
		inline const_iterator operator-(int offset) noexcept { iterator it{ *this }; return it += (-offset); }
		inline friend const_iterator operator+(int offset, const_iterator it) noexcept { return it + offset; }
		inline bool operator==(const const_iterator& o) const noexcept { return (my_node == o.my_node); }
		inline bool operator!=(const const_iterator& o) const noexcept { return !(*this == o); }
	public:
		inline const T& operator*() const noexcept { CONTAINER_VERIFY(my_node != my_cont->cend().my_node, "Can't dereference end iterator"); return my_node->val; }
		inline const T* operator->() const noexcept 
		{ CONTAINER_VERIFY(my_node != my_cont->cend().my_node, "Can't dereference end list iterator");  return std::addressof(my_node->val); }
	};
private:
	struct BaseNode {														//Узел списка
		Node* p, * n;
		inline BaseNode(Node* prev = nullptr, Node* next = nullptr) : p{ prev }, n{ next }{}
		inline BaseNode(const Node&) = delete;
		inline BaseNode& operator=(const Node&) = delete;
		inline ~BaseNode() = default;
	};
	struct Node : public BaseNode { 
		T val;		
		template<class ...Types>											//Передача аргумента с сохранением value category				
		inline Node(Node* prev, Node* next, Types&&... Args) : BaseNode(prev, next), val(std::forward<Types>(Args)...) {}
		inline Node(const Node&) = delete;
		inline Node& operator=(const Node&) = delete;
		inline ~Node() = default;
	};

	class ChainBuilder {													//Вспомогательный класс для построения цепочек из узлов
	public:
		Node* chain_head, * chain_tail;
	public:
		inline ChainBuilder() noexcept : chain_head{ nullptr }, chain_tail{ nullptr }{}
		inline ChainBuilder(Node* new_link) noexcept : ChainBuilder() { attach(new_link); }
		inline ChainBuilder(const ChainBuilder& o) noexcept = default;
		inline ChainBuilder& operator=(const ChainBuilder& o) noexcept = default;
		inline ~ChainBuilder() noexcept = default;
	public:
		inline Node* head() noexcept { return chain_head; }
		inline Node* tail() noexcept { return chain_tail; }
		inline void reset() noexcept  { chain_head = nullptr; chain_tail = nullptr; }
		inline void setChain(Node* new_head, Node* new_tail) noexcept { chain_head = new_head; chain_tail = new_tail; }
		void attach(Node* new_link) noexcept;									//Привязка узла к концу цепочки
		void close(Node* leader, Node* closer) noexcept;						//Привязка концов цепочки в двум переданным узлам
	};

	template<class Predicate>
	class BSTSortHelper {														//Вспомогательный класс для сортировки
	private:
		Node* root;
		int bst_size;
		Predicate insert_cond;													//Условие вставки				
	public:	
		inline BSTSortHelper(Predicate comparator) noexcept : root{ nullptr }, bst_size{ 0 }, insert_cond{ comparator } {}
		inline ~BSTSortHelper() noexcept = default;
		inline int size() const noexcept { return size; }	
		void insert(Node*) noexcept;											//Вставка узла в дерево. Направление ветвления зависит от функции-компаратора!		
		void traverse(ChainBuilder& cb) noexcept;								//Инфиксный (in-order) обход дерева с построением цепочки из узлов
	};
};

template <class T>
MyLinkedList<T>::MyLinkedList(size_t count)
	: MyLinkedList() {
	detach_helper();
	ChainBuilder cb;
	for (size_t i = 0; i < count; ++i)
		cb.attach(create_node(nullptr, nullptr));
	cb.close(my_end, my_end);
	my_size = count;
}

template <class T>
void MyLinkedList<T>::detach_helper(Node** first_target, Node** second_target) {
	alc = std::make_shared<Allocator>();
	alc->reserve(my_size + 1);
	Node* new_end{ create_base_node() };
	std::pair<Node*, Node*> targets{ first_target ? *first_target : nullptr, second_target ? *second_target : nullptr };
	if (my_size)
	{
		ChainBuilder cb;
		detach_copy_helper(cb, my_end->n, my_end, targets);
		cb.close(new_end, new_end);
	}
	if (first_target)
		if (*first_target == my_end)
			*first_target = new_end;
		else
			*first_target = targets.first;
	if (second_target)
		if(*second_target == my_end)
			*second_target = new_end;
		else
			*second_target  = targets.second;
	my_end = new_end;
}

template <class T>
void MyLinkedList<T>::detach_copy_helper(ChainBuilder& cb, Node* begin, Node* end, std::pair<Node*, Node*>& targets)
{
	for (Node* cur = begin; cur != end; cur = cur->n)
	{
		cb.attach(create_node(nullptr, nullptr, cur->val));
		if (cur == targets.first)
			targets.first = cb.tail();
		if (cur == targets.second)
			targets.second = cb.tail();
	}
}

template <class T>
template <class Container>
void MyLinkedList<T>::copy_container(const Container& cont) {
	ChainBuilder cb;
	alc->reserve(cont.size());
	copy_helper(cb, cont.begin(), cont.end());
	cb.close(my_end->n, my_end);
	my_size = cont.size();
}

template <class T>
void MyLinkedList<T>::move_list(MyLinkedList<T>&& o) noexcept {						//Перемещает элементы из старого списка в новый
	if (!o.empty())
	{
		std::swap(my_end, o.my_end);										
		my_size = o.my_size;
		o.my_size = 0;														//Устанавливаем новый размер
		std::swap(alc, o.alc);												//Прикрепляемся к новому хранилищу без копирования
	}
}

template <class T>
MyLinkedList<T>& MyLinkedList<T>::operator=(const MyLinkedList<T>& o) {
	if (alc != o.alc)
	{
		if (!is_shared())
			clear();
		alc = o.alc;
		my_end = o.my_end;
		my_size = o.my_size;
	}
	return *this;
}

template <class T>
MyLinkedList<T>& MyLinkedList<T>::operator=(MyLinkedList<T>&& o) noexcept {
	if (alc != o.alc) 
	{	
		if (!is_shared())
			clear();
		else
		{
			my_end = nullptr;
			my_size = 0;
		}
		move_list(std::move(o)); 
	} 
	return *this;
}

template <class T>
void MyLinkedList<T>::pop_back() noexcept{
	CONTAINER_VERIFY(!(empty()), "Empty list");						//Если список пуст, программа аварийно завершит работу
	if (is_shared())
		detach_helper();
	displace_helper(my_end->p);
	--my_size;
}

template <class T>
void MyLinkedList<T>::pop_front() noexcept{
	CONTAINER_VERIFY(!(empty()), "Empty list");
	if (is_shared())
		detach_helper();
	displace_helper(my_end->n);
	--my_size;
}

template <class T>
template<class ...Types>
typename MyLinkedList<T>::iterator MyLinkedList<T>::emplace(iterator before, Types&&... Args) {
	CONTAINER_VERIFY(before.my_cont == this, "Can't insert into another container");
	if (is_shared())
		detach_helper(std::addressof(before.my_node));
	emplace_helper(before.my_node->p, before.my_node, std::forward<Types>(Args)...);
	++my_size;
	return --before;
}

template <class T>
template<class InputIt> 
typename MyLinkedList<T>::iterator MyLinkedList<T>::insert(iterator before, InputIt first, InputIt last) {
	CONTAINER_VERIFY(before.my_cont == this, "Can't insert into another container");
	if (first != last) {
		ChainBuilder cb;
		my_size += copy_helper(cb, first, last);
		cb.close(before.my_node->p, before.my_node);
		return --before;
	}
	return before;
}

template <class T>
typename MyLinkedList<T>::iterator MyLinkedList<T>::erase(typename MyLinkedList<T>::iterator target) noexcept {
	CONTAINER_VERIFY(target.my_node != target.my_cont->cend().my_node, "Can't delete end element");
	CONTAINER_VERIFY(target.my_cont == this, "Can't erase from another container");
	if (is_shared())
		detach_helper(std::addressof(target.my_node));
	iterator after_target{ target + 1 };
	displace_helper(target.my_node);
	--my_size;
	return after_target;
}

template <class T>
typename MyLinkedList<T>::iterator MyLinkedList<T>::erase(typename MyLinkedList<T>::iterator first, typename MyLinkedList<T>::iterator last) noexcept {
	CONTAINER_VERIFY(first.my_node != first.my_cont->cend().my_node, "Can't delete end element");
	CONTAINER_VERIFY(first.my_cont == last.my_cont, "Can't erase by iterators from different containers");
	CONTAINER_VERIFY(first.my_cont == this, "Can't erase from another container");
	if (first != last) {
		if (is_shared())
			detach_helper(std::addressof(first.my_node), std::addressof(last.my_node));
		size_t count{ 0 };
		Node *first_prev{ first.my_node->p };
		my_size -= delete_helper(first.my_node, last.my_node->p);
		first_prev->n = last.my_node;
		last.my_node->p = first_prev;
	}
	return last;
}

template <class T>
bool MyLinkedList<T>::contains(const T& val) const noexcept {					
	for (const_iterator it = cbegin(); it != cend(); ++it)
		if (*it == val)
			return true;
	return false;
}

template <class T>
size_t MyLinkedList<T>::count(const T& val) const noexcept {
	size_t count{ 0 };
	for (const_iterator it = cbegin(); it != cend(); ++it)
		if (*it == val)
			++count;
	return count;
}

template <class T>
template<class Predicate>
size_t MyLinkedList<T>::remove_helper(Predicate pred) {
	if (is_shared())
		detach_helper();
	Node* target{ my_end->n };
	size_t count{ 0 };
	ChainBuilder cb;
	while (target != my_end)
	{
		if (pred(target->val))
		{
			target->p->n = target->n;
			target->n->p = target->p;
			cb.attach(target);
			++count;
		}
		target = target->n;
	}
	delete_helper(cb.head(), cb.tail());									//Даже если цепочка не содержит элементов, выполнять delete для nullptr безопасно по стандарту
	my_size -= count;														//Исправляем размер размер контейнера
	return count;
}
template <class T>
void MyLinkedList<T>::swap(MyLinkedList<T>& o) noexcept{
	if (alc != o.alc)
	{
		std::swap(my_end, o.my_end);										//Меняем местами указатели на цепочки узлов										
		std::swap(my_size, o.my_size);										//Изменяем размер
	}
}

template <class T>
MyLinkedList<T>& MyLinkedList<T>::operator+=(const MyLinkedList<T>& o) {
	if (!o.empty())
	{
		if (is_shared())
			detach_helper();
		ChainBuilder cb; 
		copy_helper(cb, o.cbegin(), o.cend());
		cb.close(my_end->p, my_end);
		my_size = my_size + o.my_size;
	}
	return *this;
}

template <class T>
bool MyLinkedList<T>::operator==(const MyLinkedList<T>& o) const {
	if (my_size != o.my_size)
		return false;
	if (alc != o.alc)																//Если хранилище общее, незачем сравнивать элементы
		for (const_iterator it = cbegin(), o_it = o.cbegin(); it != cend(); ++it, ++o_it)
			if (*it != *o_it)
				return false;
	return true;
}

template <class T>
typename MyLinkedList<T>::iterator& MyLinkedList<T>::iterator::operator++() noexcept{
#ifndef RING_LIST
	CONTAINER_VERIFY(my_node != my_cont->cend().my_node, "Can't increment end list iterator");
#endif
	my_node = my_node->n;
	return *this;
}

template <class T>
typename MyLinkedList<T>::iterator& MyLinkedList<T>::iterator::operator--() noexcept {
#ifndef RING_LIST
	CONTAINER_VERIFY(my_node != my_cont->cbegin().my_node, "Can't decrement begin list iterator");
#endif
	my_node = my_node->p;
	return *this;
}
template <class T>
typename MyLinkedList<T>::iterator MyLinkedList<T>::iterator::operator++(int) noexcept {
#ifndef RING_LIST
	CONTAINER_VERIFY(my_node != my_cont->cend().my_node, "Can't increment end list iterator");
#endif
	MyLinkedList<T>::iterator temp{ *this };
	my_node = my_node->n;
	return temp;
}

template <class T>
typename MyLinkedList<T>::iterator MyLinkedList<T>::iterator::operator--(int) noexcept {
#ifndef RING_LIST
	CONTAINER_VERIFY(my_node != my_cont->cbegin().my_node, "Can't decrement begin list iterator");
#endif
	MyLinkedList<T>::iterator temp{ *this };
	my_node = my_node->p;
	return temp;
}

template <class T>
typename MyLinkedList<T>::const_iterator& MyLinkedList<T>::const_iterator::operator++() {
#ifndef RING_LIST
	CONTAINER_VERIFY(my_node != my_cont->cend().my_node, "Can't increment end list iterator");
#endif
	my_node = my_node->n;
	return *this;
}

template <class T>
typename MyLinkedList<T>::const_iterator& MyLinkedList<T>::const_iterator::operator--() {
#ifndef RING_LIST
	CONTAINER_VERIFY(my_node != my_cont->cbegin().my_node, "Can't decrement begin list iterator");
#endif
	my_node = my_node->p;
	return *this;
}
template <class T>
typename MyLinkedList<T>::const_iterator MyLinkedList<T>::const_iterator::operator++(int) {
#ifndef RING_LIST
	CONTAINER_VERIFY(my_node != my_cont->cend().my_node, "Can't increment end list iterator");
#endif
	MyLinkedList<T>::iterator temp{ *this };
	temp.my_node = my_node->n;
	return *this;
}

template <class T>
typename MyLinkedList<T>::const_iterator MyLinkedList<T>::const_iterator::operator--(int) {
#ifndef RING_LIST
	CONTAINER_VERIFY(my_node != my_cont->cbegin().my_node->n, "Can't decrement begin list iterator");
#endif
	MyLinkedList<T>::iterator temp{ *this };
	temp.my_node = my_node->p;
	return *this;
}


template<class T>																		//Итераторы станут указывать на другой элемент!
void MyLinkedList<T>::swap(typename MyLinkedList<T>::iterator first, typename MyLinkedList<T>::iterator second) noexcept {
	if (first != second)
	{
		CONTAINER_VERIFY(first.my_node != first.my_cont->cend().my_node && second.my_node != second.my_cont->cend().my_node, "Can't swap end element");

		if (first.my_cont == second.my_cont)
		{
			if (first.my_cont->is_shared())
				first.my_cont->detach_helper(std::addressof(first.my_node), std::addressof(second.my_node));
			Node* left{ first.my_node }, *right{ second.my_node };
			if (left->p == right)
				std::swap(left, right);
			Node* left_prev{ left->p }, * right_next{ right->n };

			left->p->n = right;
			right->n->p = left;

			if (right == left->n)
			{
				left->p = right;
				right->n = left;
			}
			else
			{
				left->p = right->p;
				right->n = left->n;
				right->p->n = left;
				left->n->p = right;
			}
			left->n = right_next;
			right->p = left_prev;
		}
		else
		{
			if (first.my_cont->is_shared())
				first.my_cont->detach_helper(std::addressof(first.my_node));
			if (second.my_cont->is_shared())
				second.my_cont->detach_helper(std::addressof(second.my_node));
			std::swap(first.my_node->val, second.my_node->val);
		}
	}
}

template<class T>
template<class Predicate>												//Сортировка от begin включительно до end не включительно
void MyLinkedList<T>::sort(MyLinkedList<T>::iterator begin, MyLinkedList<T>::iterator end, Predicate comparator) {
	CONTAINER_VERIFY(begin.my_cont == this && end.my_cont == this, "Can't sort by iterators from another container");
	if (begin != end)
	{
		if (begin.my_node == my_end->n && end.my_node == my_end)
			sort(comparator);											//Сортировка слиянием работает быстрее
		else															
		{																//Проверка 
			if (is_shared())
				detach_helper(std::addressof(begin.my_node), std::addressof(end.my_node));
			tree_sort(begin, end, comparator);
		}			
	}
}

template<class T>														
template<class Predicate>
void MyLinkedList<T>::tree_sort(typename MyLinkedList<T>::iterator& begin, typename MyLinkedList<T>::iterator& end, Predicate comparator) {
	Node* leader{ begin.my_node->p };									//Отрезок [begin; end-1] будет разобран
	ChainBuilder cb;
	BSTSortHelper<Predicate> bst(comparator);
	while (begin != end)
		bst.insert((begin++).my_node);									//Сборка бинарного дерева из узлов списка
	bst.traverse(cb);
	cb.close(leader, end.my_node);										//Прикрепляем отсортированную цепочку к границам сортируемого интервала (begin-1; end)
}
	
template<class T>															//Сортировка слиянием. Может применяться только для сортировки списка целиком,
template<class Predicate>													//т.к. требует размер сортируемого участка
typename MyLinkedList<T>::Node* MyLinkedList<T>::merge_sort(Node* first, Node* last, size_t size, Predicate comparator) {
	if (size < 2)
		return first;														
	Node* middle{ first };
	for (size_t half_size = size >> 1; half_size > 0; --half_size)
		middle = middle->n;
	first = merge_sort(first, middle, size >> 1, comparator);
	middle = merge_sort(middle, last, size  - (size >> 1), comparator);
	Node* new_first{ first };

	if (PRED_TO_BOOL(comparator, middle->val, first->val))
	{
		new_first = middle;
		middle = merge_helper(first, middle);
		if (middle == last)
			return new_first;
	}
	else
	{
		first = first->n;
		if (first == middle)
			return new_first;
	}
	for (;;)
	{
		if (PRED_TO_BOOL(comparator, middle->val, first->val))
		{
			middle = merge_helper(first, middle);
			if (middle == last)
				return new_first;
		}
		else
		{
			first = first->n;
			if (first == middle)
				return new_first;
		}
	}
}
template<class T>											
typename MyLinkedList<T>::Node* MyLinkedList<T>::merge_helper(Node* first, Node* second) {
	Node* const last{ second->n }, * const second_prev = { second->p };
	second_prev->n = last;
	second->n = first;

	Node* const first_prev{ first->p };
	first_prev->n = second;

	first->p = second;
	last->p = second_prev;
	second->p = first_prev;

	return last;
}

template<class T>
template<class Predicate>
void MyLinkedList<T>::BSTSortHelper<Predicate>::insert(Node* new_node) noexcept {
	CONTAINER_VERIFY(new_node, "Can't insert empty node");
	new_node->p = nullptr;												//Теперь это узлы дерева, а не списка - старые связи им не понадобятся
	new_node->n = nullptr;												//p будет выполнять роль указателя на левого потомка, n - на правого
	if (!root)
		root = new_node;
	else
	{
		Node* parent, * child{ root };
		while (child)
		{
			parent = child;
			if (PRED_TO_BOOL(insert_cond, new_node->val, child->val))
				child = child->p;
			else
				child = child->n;
		}
		if (PRED_TO_BOOL(insert_cond, new_node->val, parent->val))
			parent->p = new_node;
		else 
			parent->n = new_node;
	}
	++bst_size;
}

template<class T>
template<class Predicate>
void MyLinkedList<T>::BSTSortHelper<Predicate>::traverse(ChainBuilder& cb) noexcept {
	std::function<void(Node*)> builder{
		[&cb, &builder](Node* node) {
		if (node->p)
			builder(node->p);
		cb.attach(node);
		if (node->n)
			builder(node->n);
	} };
	builder(root);															//Обходим дерево и строим из его узлов цепочку
}

template<class T>
void MyLinkedList<T>::ChainBuilder::attach(Node* new_link) noexcept {
	if (chain_tail)
	{
		chain_tail->n = new_link;
		new_link->p = chain_tail;
	}
	else
		chain_head = new_link;
	chain_tail = new_link;
}

template<class T>
void MyLinkedList<T>::ChainBuilder::close(Node* leader, Node* closer) noexcept {
	CONTAINER_VERIFY(chain_head && chain_tail, "Can't close empty chain");
	chain_tail->n = closer;
	closer->p = chain_tail;
	chain_head->p = leader;
	leader->n = chain_head;
}
#endif	//MyLinkedList_H