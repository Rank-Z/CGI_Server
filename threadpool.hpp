
#include<pthread.h>
#include<queue>
#include<assert.h>
#include"helper.hpp"

template<typename Work>
class threadpool
{
public:
	threadpool(int thread_number, int max_work)
		:thread_number_(thread_number), max_works_(max_work),
		threads_(nullptr), run_(true)
	{
		assert((thread_number_>0)&&(max_work>0));

		threads_=new pthread_t [thread_number_];
		if (threads_==nullptr)
			throw "threads init wrong";

		for (int i=0; i!=thread_number_; ++i)
		{
			if (pthread_create(&threads_ [i], NULL, thread_run, this)!=0)
			{
				delete[] threads_;
				throw "thread create wrong";
			}
			if (pthread_detach(threads_ [i]))
			{
				delete[] threads_;
				throw "thread detach wrong";
			}
		}
	}

	static void* thread_run(void* arg)
	{
		threadpool* self=reinterpret_cast<threadpool*>(arg);
		self->run();
		return self;
	}

	bool append(Work* new_work)
	{
		mutex_.lock();
		if (works_.size()>=max_works_)
		{
			mutex_.unlock();
			return false;
		}
		if (new_work==nullptr)
		{
			return false;
		}
		works_.push(new_work);
		mutex_.unlock();
		sem_.post();
		return true;
	}

	void run()
	{
		while (run_)
		{
			sem_.wait();
			mutex_.lock();
			if (works_.empty())
			{
				mutex_.unlock();
				continue;
			}
			Work* cur_work=works_.front();
			works_.pop();
			mutex_.unlock();
			if (cur_work!=nullptr)
			{
				cur_work->run();
			}
			delete cur_work;
		}
	}

private:
	int thread_number_;
	int max_works_;
	pthread_t* threads_;
	sem sem_;
	mutex mutex_;
	bool run_;
	std::queue<Work*> works_;
};