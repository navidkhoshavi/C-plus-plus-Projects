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


// ********** Global lock for Coarse grain implementation *************
class globalLock{
    public:
    std::vector<int> test;
    int rOperation = rand()%4;
    std::mutex share, element;
    globalLock() : test(1000, 0){}
    void RandomOperation (int &a);
    void InsertIfContains_GL(int &a, int &b);
    void DeleteThenInsert_GL(int &a, int &b);
    bool isExist(int &a);
};

void globalLock::RandomOperation (int &a)
{
    // 1: a+a
    // 2: a-a
    // 3: a*a
    // 4: a/a
    // default: ++a

    switch(rOperation)
    {
    case 1:
        element.lock();
        a = a+a;
        element.unlock();
        break;
    case 2:
        element.lock();
        a = a-a;
        element.unlock();
        break;
    case 3:
        element.lock();
        a = a*a;
        element.unlock();
        break;
    case 4:
        element.lock();
        a = a/a;
        element.unlock();
        break;
    default:
        element.lock();
        a = ++a;
        element.unlock();
        break;
    }
}

//  This  operation  ensures  that  element  y exists in the set (i.e., it cannot be removed by any concurrent delete operations) at least until element x has been successfully inserted.
void globalLock::InsertIfContains_GL(int &a, int &b){
    share.lock();
    if (globalLock::isExist(b))
        test[a] = a;
    share.unlock();
}

//  The  semantics  of DeleteThenInsert(x, y) require that element y is to be inserted into the set only if  element  x  has  been  successfully  deleted  from  the  set.
void globalLock::DeleteThenInsert_GL(int &a, int &b){
    share.lock();
    if (isExist(a)){
        test[a] = 0;
        test[b] = b;
    }
    share.unlock();
}


bool globalLock::isExist(int &a){
   for(unsigned int i = 0; i < 1000; i++){
        if (test[i]==a){
            return true;
            break;
        }
   }
   return false;
}
// ----------------- End of Global Coarse grain ------------------


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
	output.open("Coarse-grain.log");
    // ******** Coarse Grain Globla Lock *********
    globalLock gl;
    int index;
    // deadlock scenario: store value of 100 random elements in array
    for(unsigned int i = 0; i < 100; i++)
    {
        index = rand() % NUM_ELEMENTS;
        gl.test[index]=index;
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
            gl.RandomOperation(x);
            gl.InsertIfContains_GL(a,b);
            gl.DeleteThenInsert_GL(c,d);

        }
    };

    std::cout << "boost threadpool test on Coarse Grain Global Lock: \n";
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

    // --------------- End of Test of Coarse Grain Global Lock ------


    return 0;
}


