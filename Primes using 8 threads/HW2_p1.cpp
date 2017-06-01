#include <iostream>
#include <stdlib.h>
#include <time.h>
#include <fstream>
#include <omp.h>

using namespace std;

int main(int argc, char *argv[])
{
    // initialization
	clock_t tStart = clock(); // to get the execution time
	int upper_band = 100000000; // upper bound
	int N;
	int total_prime = 0; // return the total number of primes
	int top10_prime_list[10]; // store the final top 10 prime in the list
	bool *list;
	N = upper_band;
	unsigned long sum_prime = 0;
	list = new bool[N];

	// create the output
	ofstream output;
	output.open("primes_8thread.txt");

// first round of algorithm executed by 8 threads
omp_set_dynamic(0);
omp_set_num_threads(8);
# pragma omp parallel for    // make this line as comment, if you want to run program with just one thread
	for (int i = 0; i < N; i++)
	{
		if (i % 2 == 0)
			list[i] = 0;
		else if (i % 2 != 0)
			list[i] = 1;
	}
	list[1] = 0; // '1' is not prime
	list[2] = 1; // '2' is prime
	int index = 2;
	int j = 0;
    int prime = 3;
	int second_round = 0;
	do {
		second_round = 3 * prime; // second round of algorithm executed by 8 threads
		omp_set_dynamic(0);
		omp_set_num_threads(8); // Sets the number of threads in subsequent parallel regions
        # pragma omp parallel for  // make this line as comment, if you want to run program with just one thread
        for (int i = second_round; i < N; i += 2 * prime)
        {
            if (list[i] == 1)
            list[i] = 0;
        }

        if (list[++index] == 1)
            prime = index;
        } while (prime*prime < upper_band);

        # pragma omp parallel for reduction (+:total_prime, sum_prime) // make this line as comment, if you want to run program with just one thread
        for (int i = 0; i < upper_band; i++)
        {
            if (list[i] == 1)
            {
                sum_prime += i;
                total_prime++;
            }
        }
        // store the top 10 primes into top10_prime_list
        for (int i = upper_band; i > 0; i--)
        {
            if (list[i] == 1)
            {
                if (j<10)
                {
                    top10_prime_list[j] = i;
                    j++;
                }
                else
                    break;
            }
        }
        // Output of the file
        output << "Time taken: " << (double)(clock() - tStart)/CLOCKS_PER_SEC << " seconds"<< endl;
        output << "The total number of primes found are "<< total_prime << endl;
        output << "The sum of all primes found is " << sum_prime + 5 << endl;
        output << "Top 10 maximum primes, listed in order from lowest to highest"<<endl;
        for (int x = 9; x >= 0; x--)
            output << top10_prime_list[x]<<endl;
        char c;
        cout << endl << "Please insert a key to exit ";
        if (cin >> c)
            return 0;
    }
