#ifndef HELPER_HPP
#define HELPER_HPP

#include<semaphore.h>
#include<pthread.h>

class sem
{
public:
	sem()
		:valid_(false)
	{
		if (sem_init(&sem_, 0, 0)==0)
			valid_=true;
	}

	~sem()
	{
		sem_destroy(&sem_);
	}

	void reset()
	{
		if (sem_init(&sem_, 0, 0)==0)
			valid_=true;
	}

	bool wait()
	{
		return (sem_wait(&sem_)==0);
	}

	bool post()
	{
		return (sem_post(&sem_)==0);
	}

private:
	sem_t sem_;
	bool valid_;
};

class mutex
{
public:
	mutex()
	{
		pthread_mutex_init(&mutex_, NULL);
	}

	~mutex()
	{
		pthread_mutex_destroy(&mutex_);
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
	explicit MutexLock(mutex& mutex)
		:mutex_(mutex)
	{
		mutex_.lock();
	}

	~MutexLock()
	{
		mutex_.unlock();
	}

private:
	mutex& mutex_;
};

#endif // !HELPER_HPP
