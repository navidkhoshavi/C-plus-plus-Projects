#ifndef __TERVEL_CONTAINERS_WF_LINKED_LIST_QUEUE_QUEUE__IMP_H_
#define __TERVEL_CONTAINERS_WF_LINKED_LIST_QUEUE_QUEUE__IMP_H_
#include <tervel/util/progress_assurance.h>

namespace tervel {
namespace containers {
namespace wf {

template<typename T>
bool Queue<T>::Node::on_watch(std::atomic<void *> *address, void *expected) {
	std::atomic<Node *> *loadedaddr =
			reinterpret_cast<std::atomic<Node *>*>(address);
	Node* node = loadedaddr->load();
	if (node == expected) {
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
void Queue<T>::Accessor::unaccess_ptr_only() {
	ptr_->unaccess();
};

template<typename T>
bool Queue<T>::Accessor::init(Node *node, std::atomic<Node *> *address) {
	assert(node != nullptr);
	assert(address != nullptr);
	node = address->load();
	bool res = true;
	if (node != nullptr) {
		res = tervel::util::memory::hp::HazardPointer::watch(kSlot, node, reinterpret_cast<std::atomic<void *> *>(address), node);
	} else
		return true;

	if (res)
		ptr_ = node;
	Node *nextelem = node->next_.load();
	typedef tervel::util::memory::hp::HazardPointer::SlotID SlotID;
	static const SlotID watch_pos = SlotID::SHORTUSE;
	if (node == address->load()) {
		res = tervel::util::memory::hp::HazardPointer::watch(watch_pos,	nextelem, reinterpret_cast<std::atomic<void *> *>(&node->next_), nextelem);
	}
	if (res)
		ptr_next_ = nextelem;
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
	tervel::util::ProgressAssurance::check_for_announcement();
	util::ProgressAssurance::Limit progAssur;
	while (!progAssur.isDelayed()) {
		Accessor access;
		Node accessNode;
		if (access.init(&accessNode, &tail_) == false) {
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

	EnqueueOp *op = new EnqueueOp(this, value);
	tervel::util::ProgressAssurance::make_announcement(op);
	op->safe_delete();
	size(1);
	return true;
}

template<typename T>
bool Queue<T>::dequeue(Accessor &access) {
	tervel::util::ProgressAssurance::check_for_announcement();
	util::ProgressAssurance::Limit progAssur;
	while (!progAssur.isDelayed()) {
		Node node;
		if (access.init(&node, &head_) ==  false) {
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
	DequeueOp *op = new DequeueOp(this);
	tervel::util::ProgressAssurance::make_announcement(op);
	bool res = op->result(access);
	if (res)
		size(-1);
	op->safe_delete();
	return res;
};

template<typename T>
bool Queue<T>::empty() {
	while (true) {
		Accessor access;
		if (this->head() == this->tail())
			break;
		dequeue(access);
	}
	return true;
}

}  // namespace wf
}  // namespace containers
}  // namespace tervel
#endif  // __TERVEL_CONTAINERS_WF_LINKED_LIST_QUEUE_QUEUE__IMP_H_
