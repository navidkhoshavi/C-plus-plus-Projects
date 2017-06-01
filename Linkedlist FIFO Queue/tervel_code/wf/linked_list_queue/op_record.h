#ifndef __TERVEL_CONTAINERS_WF_LINKED_LIST_QUEUE_OP_RECORD_H_
#define __TERVEL_CONTAINERS_WF_LINKED_LIST_QUEUE_OP_RECORD_H_

#include <tervel/util/util.h>
#include <tervel/util/progress_assurance.h>
#include <tervel/util/memory/hp/hp_element.h>
#include <tervel/util/memory/hp/hazard_pointer.h>

#include <tervel/containers/wf/linked_list_queue/queue.h>

namespace tervel {
namespace containers {
namespace wf {

template<typename T>
class Queue<T>::Op: public util::OpRecord {
public:
	static const tervel::util::memory::hp::HazardPointer::SlotID kSlot =
			tervel::util::memory::hp::HazardPointer::SlotID::SHORTUSE2;

	class Helper: public tervel::util::memory::hp::Element {
	public:
		Helper(Op * op) :
				op_(op) {
		}
		;

		void finish(std::atomic<Node *> *address, Node *expected) {
			Node * e = expected;
			if (op_->associate(this)) {
				address->compare_exchange_strong(e, newValue_);
			} else {
				address->compare_exchange_strong(e, expValue_);
			}
		}
		;

		bool on_watch(std::atomic<void *> *address, void *expected) {
			// implement this function to acquire HazardPointer protection on op and then remove the Helper
			typedef tervel::util::memory::hp::HazardPointer::SlotID SlotID;
			const SlotID pos = SlotID::SHORTUSE2;
			bool res = tervel::util::memory::hp::HazardPointer::watch(pos, op_,
					address, expected);

			if (res) {
				finish(reinterpret_cast<std::atomic<Node *> *>(address),
						reinterpret_cast<Node *>(expected));
				tervel::util::memory::hp::HazardPointer::unwatch(pos);
			}
			op_->helper()->safe_delete();
			// must return false. (see stack for example)
			return false;
		}
		;

		bool is_valid() {
			return op_->helper() == this;
		}
		;

		bool remove(std::atomic<Node *> *address) {
			// TODO: implement this function, determine the logical value of the helper and then replace.
			return false;
		}
		;

		Op * const op_;
		Node * newValue_;
		Node * expValue_;
		void expValue(Node *e) {
			expValue_ = e;
		}
		;
		void newValue(Node *e) {
			newValue_ = e;
		}
		;
	};

	Op(Queue<T> *queue) :
			queue_(queue) {
	}
	;

	~Op() {
		//  implement, free helper
		delete helper_;
	}
	;

	void fail() {
		associate(fail_val_);
	}
	;

	bool notDone() {
		return helper() == nullptr;
	}
	;

	bool on_is_watched() {
		// implement this function, it should return if the helper object is watched
		Helper *h = helper_.load();
		assert(h != nullptr);
		if (h != fail_val_) {
			bool res = tervel::util::memory::hp::HazardPointer::is_watched(h);
			return res;
		}
		return false;
	}
	;

	std::atomic<Helper *> helper_ { nullptr };
	Helper * helper() {
		return helper_.load();
	}
	;

	bool associate(Helper *h) {
		Helper * temp = nullptr;
		bool res = helper_.compare_exchange_strong(temp, h);
		return res || helper() == h;
	}
	;

	static constexpr Helper * fail_val_ = reinterpret_cast<Helper *>(0x1L);

	Queue<T> * const queue_ { nullptr };
};

template<typename T>
class Queue<T>::DequeueOp: public Queue<T>::Op {
public:
	DequeueOp(Queue<T> *queue) :
			Op(queue) {
	}
	;

	bool result(Accessor &access) {
		typename Queue<T>::Op::Helper *helper = Queue<T>::Op::helper();

		if (helper == Queue<T>::Op::fail_val_) {
			return false;
		} else {
			// the value is already in access,so no need to load it
			return true;
		}
	}
	;


	bool notValid(typename Queue<T>::Op::Helper * h) {
		return Queue<T>::Op::helper() != h;
	}

	void help_complete() {
		// TODO: implement this function based on your LF code.
		// write a blurb in comments about why this lock-free code behaves wait-free as a result of the announcement table and how it affects new operation creation
		// // Maybe helpful: reinterpret_cast<Node *>
		typename Queue<T>::Op::Helper *helper =
				new typename Queue<T>::Op::Helper(this);
		Node *helper_marked =
				reinterpret_cast<Node *>(tervel::util::set_1st_lsb_1<typename Queue<T>::Op::Helper>(
						helper));

		while (DequeueOp::notDone()) {
			Accessor access;
			//  implement
			// Dont forget the case where the queue is empty
			Node * node = new Node();
			if (access.init(node, &DequeueOp::queue_->head_) == false) {
				continue;
			}
			Node *first = access.ptr();
			Node *last = DequeueOp::queue_->tail();
			Node *next = access.ptr_next();

			if(first == nullptr){
				DequeueOp::fail();
				break;
			}

			helper->newValue_ = next;

			if (first == DequeueOp::queue_->head()) {
				if (first == last) {
					if (next == nullptr) {
						DequeueOp::fail();
						break;
					}
					if (DequeueOp::queue_->tail_.compare_exchange_strong(last,
							helper_marked)) {
						helper->finish(&(DequeueOp::queue_->tail_),
								helper_marked);
						assert(
								DequeueOp::queue_->tail_.load()
										!= helper_marked);
						if (DequeueOp::notValid(helper)) {
							helper->safe_delete();
						}
						return;
					}
				} else {
					if (DequeueOp::queue_->head_.compare_exchange_strong(last,
							helper_marked)) {
						helper->finish(&(DequeueOp::queue_->head_),
								helper_marked);
						assert(
								DequeueOp::queue_->head_.load()
										!= helper_marked);
						if (DequeueOp::notValid(helper)) {
							helper->safe_delete();
						}
						return;
					}
				}
			}

		}  // End while
	}
	;

	T value_;
	// DISALLOW_COPY_AND_ASSIGN(DequeueOp);
};
// class

template<typename T>
class Queue<T>::EnqueueOp: public Queue<T>::Op {
public:
	EnqueueOp(Queue<T> *queue, T &value) :
			Op(queue) {
		value_ = value;
	}

	bool notValid(typename Queue<T>::Op::Helper * h) {
		return Queue<T>::Op::helper() != h;
	}

	void help_complete() {
		// implement this function based on your LF code.
		// Maybe helpful: reinterpret_cast<Node *>
		typename Queue<T>::Op::Helper *helper =
				new typename Queue<T>::Op::Helper(this);
		helper->newValue_ = new Node(this->value_);
		Node *helper_marked =
				reinterpret_cast<Node *>(tervel::util::set_1st_lsb_1<typename Queue<T>::Op::Helper>(
						helper));

		Node *node = new Node();
		while (EnqueueOp::notDone()) {
			Accessor access;
			if (access.init(node, &EnqueueOp::queue_->tail_) == false) {
				continue;
			}
			Node *last = access.ptr();
			Node *next = access.ptr_next();
			if (last == EnqueueOp::queue_->tail()) {
				if (next == nullptr) {
					if (EnqueueOp::queue_->tail_.compare_exchange_strong(last,
							helper_marked)) {
						helper->finish(&(EnqueueOp::queue_->tail_),
								helper_marked);
						assert(
								EnqueueOp::queue_->tail_.load()
										!= helper_marked);
						if (EnqueueOp::notValid(helper)) {
							helper->safe_delete();
						}
						return;
					}
				} else {
					if (EnqueueOp::queue_->tail_.compare_exchange_strong(last,
							next)) {
						helper->finish(&(EnqueueOp::queue_->tail_),
								next);
						assert(
								EnqueueOp::queue_->tail_.load()
										!= next);
						if (EnqueueOp::notValid(helper)) {
							helper->safe_delete();
						}
						return;
					}
				}
			}
		}
	}
	;

	T value_;
	// DISALLOW_COPY_AND_ASSIGN(EnqueueOp);
};
// class

}// namespace wf
}  // namespace containers
}  // namespace tervel

#endif  // __TERVEL_CONTAINERS_WF_LINKED_LIST_QUEUE_OP_RECORD_H_
