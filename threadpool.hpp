#ifndef THREADPOOL_HPP
#define THREADPOOL_HPP

#include<pthread.h>
#include<queue>
#include<memory>
#include<assert.h>
#include"helper.hpp"

template<typename Work>
class threadpool
{
	static bool run_;
public:
	threadpool(int, int);

	~threadpool()
	{
		delete[] threads_;
	}

	static void* thread_run(void* arg)
	{
		threadpool* self=reinterpret_cast<threadpool*>(arg);
		self->run();
		return self;
	}

	bool append(std::shared_ptr<Work> new_work)
	{
		MutexLock mutexlock(mutex_);
		if (works_.size()>=max_works_)
		{
			return false;
		}
		if (!new_work)
		{
			return false;
		}
		works_.push(new_work);
		cond_.signal();
		return true;
	}

	std::shared_ptr<Work> get_work()
	{
		MutexLock mutexlock(mutex_);
		while (works_.empty())
		{
			cond_.wait();
		}
		auto ret=works_.front();
		works_.pop();
		return ret;
	}

	void run()
	{
		while (run_)
		{
			auto cur_work=get_work();
			if (cur_work)
			{
				cur_work->run();
			}
		}

	}

	void set_stop()
	{
		run_=false;
	}

private:
	int thread_number_;
	int max_works_;
	pthread_t* threads_;
	Mutex mutex_;
	Cond cond_;
	std::queue<std::shared_ptr<Work>> works_;
};

template<typename Work>
threadpool<Work>::threadpool(int thread_number, int max_work)
	:thread_number_(thread_number), max_works_(max_work),
	threads_(nullptr),mutex_(),cond_(mutex_)
{
	assert((thread_number_>0)&&(max_work>0));

	threads_=new pthread_t [thread_number_];
	if (threads_==nullptr)
		throw "threads init failed";

	run_=true;

	for (int i=0; i!=thread_number_; ++i)
	{
		if (pthread_create(&threads_ [i], NULL, thread_run, this)!=0)
		{
			delete[] threads_;
			throw "thread create failed";
		}
		if (pthread_detach(threads_ [i]))
		{
			delete[] threads_;
			throw "thread detach failed";
		}
	}
}

template<typename Work>
bool threadpool<Work>::run_=true;

#endif // !THREADPOOL_HPP
