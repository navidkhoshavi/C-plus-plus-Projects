#ifndef __TERVEL_CONTAINERS_LF_LINKED_LIST_QUEUE_QUEUE__IMP_H_
#define __TERVEL_CONTAINERS_LF_LINKED_LIST_QUEUE_QUEUE__IMP_H_
namespace tervel {
namespace containers {
namespace lf {

template<typename T>
bool Queue<T>::Node::on_watch(std::atomic<void *> *address, void *expected) {
	std::atomic<Node *> *loadedaddr = reinterpret_cast<std::atomic<Node *>*>(address);
	Node* node = loadedaddr->load();
	if(node == expected){
		node->access();
		return true;
	}

	return false;
}

// template<typename T>
// void
// Queue<T>::Node::on_unwatch(std::atomic<void *> *address, void *expected) {
// // Do not change. //
// Un comment if necessary
//   return;
// }

template<typename T>
bool Queue<T>::Node::on_is_watched() {
	return is_accessed();
}

template<typename T>
void Queue<T>::Accessor::uninit() {
	tervel::util::memory::hp::HazardPointer::unwatch(kSlot);
};

template<typename T>
void Queue<T>::Accessor::unaccess_node_only() {
	node_->unaccess();
};

/**
 * @brief This function is used to safely access the value stored at an address.
 *
 * @details Using Tervel's framework, load the value of `address' and attempt to acquire memory protection.
 *
 *
 * @param node [description]
 * @param address the address to load from
 * @tparam T Data type stored in the queue
 * @return whether or not init was success
 */
template<typename T>
bool Queue<T>::Accessor::init(Node *node, std::atomic<Node *> *address) {
	assert(node != nullptr);
	node = address->load();
	bool res = true;
	if (node != nullptr) {
		res = tervel::util::memory::hp::HazardPointer::watch(kSlot, node, reinterpret_cast<std::atomic<void *> *>(address), node);
	} else
		return true;

	if (res)
		node_ = node;
	Node *nextelem = node->next_.load();
	typedef tervel::util::memory::hp::HazardPointer::SlotID SlotID;
	static const SlotID watch_pos = SlotID::SHORTUSE;
	if (node == address->load()) {
		res = tervel::util::memory::hp::HazardPointer::watch(watch_pos, nextelem, reinterpret_cast<std::atomic<void *> *>(&node->next_), nextelem);
	}

	if (res)
		next_ = nextelem;
	return res;
};

template<typename T>
Queue<T>::Queue() {
	Node * node = new Node();
	head_.store(node);
	tail_.store(node);
}

template<typename T>
Queue<T>::~Queue() {
	empty();
}

template<typename T>
bool Queue<T>::enqueue(T &value) {
	Node *node = new Node(value);
	while (true) {
		Accessor access;
		Node tailnode_;
		if (access.init(&tailnode_, &tail_) == false) {
			continue;
		}
		Node *last = access.ptr();
		Node *next = access.ptr_next();
		if (last == tail()) {
			if (next == nullptr) {
				if (last->next_.compare_exchange_strong(next, node)) {
					set_tail(last, node);
					return true;
				}
			} else {
				set_tail(last, next);
			}
		}
	}
}

template<typename T>
bool Queue<T>::dequeue(Accessor &access) {
	while (true) {
		Node headnode_;
		if(access.init(&headnode_,&head_) == false){
			continue;
		}
		Node *first = access.ptr();
		Node *last = tail();
		Node *next = access.ptr_next();
		if (first == head()) {
			if (first == last) {
				if (next == nullptr) {
					return false;
				}
				set_tail(last, next);
			} else {
				if (set_head(first, next)) {
					return true;
				}
			}
		}
	}
};

template<typename T>
bool Queue<T>::empty() {
	while (true) {
		Accessor *access = new Accessor();
		if(head() == tail())
			break;
		dequeue(*access);
		delete access;
	}
	return true;
}

}  // namespace lf
}  // namespace containers
}  // namespace tervel
#endif  // __TERVEL_CONTAINERS_LF_LINKED_LIST_QUEUE_QUEUE__IMP_H_
