#ifndef HELPER_HPP
#define HELPER_HPP

#include<pthread.h>

class Mutex
{
public:
	Mutex()
	{
		pthread_mutex_init(&mutex_, NULL);
	}

	~Mutex()
	{
		pthread_mutex_destroy(&mutex_);
	}

	pthread_mutex_t& get_mutex()
	{
		return mutex_;
	}

	bool lock()
	{
		return (pthread_mutex_lock(&mutex_)==0);
	}

	bool unlock()
	{
		return (pthread_mutex_unlock(&mutex_)==0);
	}

private:
	pthread_mutex_t mutex_;
};


class MutexLock // RAII
{
public:
	explicit MutexLock(Mutex& mutex)
		:mutex_(mutex)
	{
		mutex_.lock();
	}

	~MutexLock()
	{
		mutex_.unlock();
	}

private:
	Mutex& mutex_;
};


class Cond
{
public:
	explicit Cond(Mutex& mutex)
		:mutex_(mutex)
	{
		if (pthread_cond_init(&cond_, NULL)!=0)
		{
			throw "cond cond_ initializztion failed";
		}
	}

	~Cond()
	{
		pthread_cond_destroy(&cond_);
	}

	void wait()
	{
		pthread_cond_wait(&cond_,&mutex_.get_mutex());
	}

	void signal()
	{
		pthread_cond_signal(&cond_);
	}

private:
	Mutex& mutex_;
	pthread_cond_t cond_;
};

#endif // !HELPER_HPP
