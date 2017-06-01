#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <functional>
#include <fstream>
#include <cstdlib>
#include <chrono>
#include <ctime>
#include <boost/asio/io_service.hpp>
#include <boost/thread.hpp>
#include <boost/bind.hpp>

// ********** Two-phase locking mechanism *************
class twoPhaseLock
{
public:
    std::vector<int> test;
    int rOperation = rand()%4;
    std::mutex test_mutexes[1000];
    twoPhaseLock() : test(1000, 0) {}
    void RandomOperation(int &a);
    void InsertIfContains_2PL(int &a, int &b);
    void DeleteThenInsert_2PL(int &a, int &b);
    bool isExist(int &a);
};

void twoPhaseLock::RandomOperation (int &a)
{
    // 1: a+a
    // 2: a-a
    // 3: a*a
    // 4: a/a
    // default: ++a

    switch(rOperation)
    {
    case 1:
        test_mutexes[a].lock();
        a = a+a;
        test_mutexes[a].unlock();
        break;
    case 2:
        test_mutexes[a].lock();
        a = a-a;
        test_mutexes[a].unlock();
        break;
    case 3:
        test_mutexes[a].lock();
        a = a*a;
        test_mutexes[a].unlock();
        break;
    case 4:
        test_mutexes[a].lock();
        a = a/a;
        test_mutexes[a].unlock();
        break;
    default:
        test_mutexes[a].lock();
        a = ++a;
        test_mutexes[a].unlock();
        break;
    }
}

void twoPhaseLock::InsertIfContains_2PL(int &a, int &b)
{
    test_mutexes[b].lock();
    test_mutexes[a].lock();
    if (twoPhaseLock::isExist(b))
        test[a] = a;
    test_mutexes[a].unlock();
    test_mutexes[b].unlock();
}

void twoPhaseLock::DeleteThenInsert_2PL(int &a, int &b)
{
    test_mutexes[a].lock();
    test_mutexes[b].lock();
    if (twoPhaseLock::isExist(a))
    {
        test[a] = 0;
        test[b] = b;
    }
    test_mutexes[b].unlock();
    test_mutexes[a].unlock();
}

bool twoPhaseLock::isExist(int &a)
{
    for(unsigned int i = 0; i < 1000; i++)
    {
        if (test[i]==a)
        {
            return true;
            break;
        }
    }
    return false;
}
// ------- End of two-phase locking mechanism -----------


int main()
{
    const int NUM_ACCESSES = 10;
    const int NUM_ELEMENTS = 1000;
    const int NUM_THREADS = 2;
    const int NUM_RUNS = 100;
    boost::asio::io_service ioService;
    boost::thread_group threadpool;

    srand(time(NULL));
    int average = 0;

	std::ofstream output;
	output.open("2pl.log");

    // ******** two-phase Lock *********
    twoPhaseLock tpl;
    int index;
    std::cout << "boost threadpool test on two-phase Lock implementation \n";
    // deadlock scenario: store value of 100 random elements in array
    for(unsigned int i = 0; i < 100; i++)
    {
        index = rand() % NUM_ELEMENTS;
        tpl.test[index]=index;
    }
    std::function<void()> func1 = [&]()
    {
        int a, b, c, d, x;
        for(unsigned int i = 0; i < NUM_ACCESSES/NUM_THREADS; i++)
        {
            x = rand() % NUM_ELEMENTS;
            a = rand() % NUM_ELEMENTS;
            b = rand() % NUM_ELEMENTS;
            c = rand() % NUM_ELEMENTS;
            d = rand() % NUM_ELEMENTS;
            tpl.RandomOperation(x);
            tpl.InsertIfContains_2PL(a,b);
            tpl.DeleteThenInsert_2PL(c,d);

        }
    };

    boost::asio::io_service::work work(ioService);

    for(int i = 0; i < NUM_THREADS; i++)
        boost::bind(&boost::asio::io_service::run, &ioService);

    for(int i = 0; i < NUM_RUNS; i++)
    {
        // Compute average of 10 runs
        for (int k=0; k < 10; k++)
        {
            std::chrono::time_point<std::chrono::steady_clock> start, end;
            start = std::chrono::steady_clock::now();
            for(int j = 0; j < (i*NUM_ACCESSES); j++)
            {
                ioService.post(boost::bind(func1));
            }

            threadpool.join_all();
            end = std::chrono::steady_clock::now();
            auto elapse = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
            //std::cout << "Elapsed time: " << elapse.count() << "ms\n";
            //output << elapse.count() << std::endl;
            average += elapse.count();
        }
        std::cout << "Average time: " << average/10 << "us\n\n";
        output << average/10 << std::endl;
    }

    // --------------- End of Test of two-phase Lock ------


    return 0;
}
