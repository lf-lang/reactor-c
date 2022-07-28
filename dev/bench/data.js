window.BENCHMARK_DATA = {
  "lastUpdate": 1659052035659,
  "repoUrl": "https://github.com/lf-lang/reactor-c",
  "entries": {
    "Lingua Franca C Benchmark -- Multithreaded": [
      {
        "commit": {
          "author": {
            "name": "lf-lang",
            "username": "lf-lang"
          },
          "committer": {
            "name": "lf-lang",
            "username": "lf-lang"
          },
          "id": "62c26925b27c32189fbba37f542c5a6f9f01389d",
          "message": "Continuous benchmarking",
          "timestamp": "2022-06-08T21:36:19Z",
          "url": "https://github.com/lf-lang/reactor-c/pull/51/commits/62c26925b27c32189fbba37f542c5a6f9f01389d"
        },
        "date": 1659051878809,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "Filter Bank (scheduler=adaptive)",
            "value": 247.54308333333336,
            "unit": "ms",
            "extra": "Target: Filter Bank\nTotal Iterations: 12\nThreads: 0\nScheduler: adaptive"
          },
          {
            "name": "Filter Bank (scheduler=GEDF_NP)",
            "value": 901.8063333333334,
            "unit": "ms",
            "extra": "Target: Filter Bank\nTotal Iterations: 12\nThreads: 0\nScheduler: GEDF_NP"
          },
          {
            "name": "Filter Bank (scheduler=NP)",
            "value": 727.3369166666666,
            "unit": "ms",
            "extra": "Target: Filter Bank\nTotal Iterations: 12\nThreads: 0\nScheduler: NP"
          },
          {
            "name": "Producer Consumer (bounded) (scheduler=adaptive)",
            "value": 150.78116666666668,
            "unit": "ms",
            "extra": "Target: Producer Consumer (bounded)\nTotal Iterations: 12\nThreads: 0\nScheduler: adaptive"
          },
          {
            "name": "Producer Consumer (bounded) (scheduler=GEDF_NP)",
            "value": 142.10341666666667,
            "unit": "ms",
            "extra": "Target: Producer Consumer (bounded)\nTotal Iterations: 12\nThreads: 0\nScheduler: GEDF_NP"
          },
          {
            "name": "Producer Consumer (bounded) (scheduler=NP)",
            "value": 138.81574999999998,
            "unit": "ms",
            "extra": "Target: Producer Consumer (bounded)\nTotal Iterations: 12\nThreads: 0\nScheduler: NP"
          },
          {
            "name": "Bank Transaction (scheduler=adaptive)",
            "value": 159.09916666666666,
            "unit": "ms",
            "extra": "Target: Bank Transaction\nTotal Iterations: 12\nThreads: 0\nScheduler: adaptive"
          },
          {
            "name": "Bank Transaction (scheduler=GEDF_NP)",
            "value": 114.39891666666665,
            "unit": "ms",
            "extra": "Target: Bank Transaction\nTotal Iterations: 12\nThreads: 0\nScheduler: GEDF_NP"
          },
          {
            "name": "Bank Transaction (scheduler=NP)",
            "value": 108.512,
            "unit": "ms",
            "extra": "Target: Bank Transaction\nTotal Iterations: 12\nThreads: 0\nScheduler: NP"
          },
          {
            "name": "Recursive Matrix Multiplication (scheduler=adaptive)",
            "value": 129.59866666666665,
            "unit": "ms",
            "extra": "Target: Recursive Matrix Multiplication\nTotal Iterations: 12\nThreads: 0\nScheduler: adaptive"
          },
          {
            "name": "Recursive Matrix Multiplication (scheduler=GEDF_NP)",
            "value": 115.93416666666668,
            "unit": "ms",
            "extra": "Target: Recursive Matrix Multiplication\nTotal Iterations: 12\nThreads: 0\nScheduler: GEDF_NP"
          },
          {
            "name": "Recursive Matrix Multiplication (scheduler=NP)",
            "value": 118.07633333333337,
            "unit": "ms",
            "extra": "Target: Recursive Matrix Multiplication\nTotal Iterations: 12\nThreads: 0\nScheduler: NP"
          },
          {
            "name": "Precise Pi Computation (scheduler=adaptive)",
            "value": 90.34841666666667,
            "unit": "ms",
            "extra": "Target: Precise Pi Computation\nTotal Iterations: 12\nThreads: 0\nScheduler: adaptive"
          },
          {
            "name": "Precise Pi Computation (scheduler=GEDF_NP)",
            "value": 88.74750000000002,
            "unit": "ms",
            "extra": "Target: Precise Pi Computation\nTotal Iterations: 12\nThreads: 0\nScheduler: GEDF_NP"
          },
          {
            "name": "Precise Pi Computation (scheduler=NP)",
            "value": 90.53575,
            "unit": "ms",
            "extra": "Target: Precise Pi Computation\nTotal Iterations: 12\nThreads: 0\nScheduler: NP"
          },
          {
            "name": "Ping Pong (scheduler=adaptive)",
            "value": 141.82500000000002,
            "unit": "ms",
            "extra": "Target: Ping Pong\nTotal Iterations: 12\nThreads: 0\nScheduler: adaptive"
          },
          {
            "name": "Ping Pong (scheduler=GEDF_NP)",
            "value": 194.63558333333333,
            "unit": "ms",
            "extra": "Target: Ping Pong\nTotal Iterations: 12\nThreads: 0\nScheduler: GEDF_NP"
          },
          {
            "name": "Ping Pong (scheduler=NP)",
            "value": 121.55825,
            "unit": "ms",
            "extra": "Target: Ping Pong\nTotal Iterations: 12\nThreads: 0\nScheduler: NP"
          },
          {
            "name": "Philosophers (scheduler=adaptive)",
            "value": 61.555416666666666,
            "unit": "ms",
            "extra": "Target: Philosophers\nTotal Iterations: 12\nThreads: 0\nScheduler: adaptive"
          },
          {
            "name": "Philosophers (scheduler=GEDF_NP)",
            "value": 225.81266666666662,
            "unit": "ms",
            "extra": "Target: Philosophers\nTotal Iterations: 12\nThreads: 0\nScheduler: GEDF_NP"
          },
          {
            "name": "Philosophers (scheduler=NP)",
            "value": 157.3833333333333,
            "unit": "ms",
            "extra": "Target: Philosophers\nTotal Iterations: 12\nThreads: 0\nScheduler: NP"
          },
          {
            "name": "Concurrent Dictionary (scheduler=adaptive)",
            "value": 96.59033333333333,
            "unit": "ms",
            "extra": "Target: Concurrent Dictionary\nTotal Iterations: 12\nThreads: 0\nScheduler: adaptive"
          },
          {
            "name": "Concurrent Dictionary (scheduler=GEDF_NP)",
            "value": 184.2530833333333,
            "unit": "ms",
            "extra": "Target: Concurrent Dictionary\nTotal Iterations: 12\nThreads: 0\nScheduler: GEDF_NP"
          },
          {
            "name": "Concurrent Dictionary (scheduler=NP)",
            "value": 172.00283333333334,
            "unit": "ms",
            "extra": "Target: Concurrent Dictionary\nTotal Iterations: 12\nThreads: 0\nScheduler: NP"
          },
          {
            "name": "Thread Ring (scheduler=adaptive)",
            "value": 116.87491666666669,
            "unit": "ms",
            "extra": "Target: Thread Ring\nTotal Iterations: 12\nThreads: 0\nScheduler: adaptive"
          },
          {
            "name": "Thread Ring (scheduler=GEDF_NP)",
            "value": 176.287,
            "unit": "ms",
            "extra": "Target: Thread Ring\nTotal Iterations: 12\nThreads: 0\nScheduler: GEDF_NP"
          },
          {
            "name": "Thread Ring (scheduler=NP)",
            "value": 106.17491666666666,
            "unit": "ms",
            "extra": "Target: Thread Ring\nTotal Iterations: 12\nThreads: 0\nScheduler: NP"
          },
          {
            "name": "Cigarette Smokers (scheduler=adaptive)",
            "value": 2.2062500000000003,
            "unit": "ms",
            "extra": "Target: Cigarette Smokers\nTotal Iterations: 12\nThreads: 0\nScheduler: adaptive"
          },
          {
            "name": "Cigarette Smokers (scheduler=GEDF_NP)",
            "value": 2.3310833333333334,
            "unit": "ms",
            "extra": "Target: Cigarette Smokers\nTotal Iterations: 12\nThreads: 0\nScheduler: GEDF_NP"
          },
          {
            "name": "Cigarette Smokers (scheduler=NP)",
            "value": 2.17775,
            "unit": "ms",
            "extra": "Target: Cigarette Smokers\nTotal Iterations: 12\nThreads: 0\nScheduler: NP"
          },
          {
            "name": "A-Star Search (scheduler=adaptive)",
            "value": 377.7347692307693,
            "unit": "ms",
            "extra": "Target: A-Star Search\nTotal Iterations: 12\nThreads: 0\nScheduler: adaptive"
          },
          {
            "name": "A-Star Search (scheduler=GEDF_NP)",
            "value": 519.3708461538462,
            "unit": "ms",
            "extra": "Target: A-Star Search\nTotal Iterations: 12\nThreads: 0\nScheduler: GEDF_NP"
          },
          {
            "name": "A-Star Search (scheduler=NP)",
            "value": 418.8482307692307,
            "unit": "ms",
            "extra": "Target: A-Star Search\nTotal Iterations: 12\nThreads: 0\nScheduler: NP"
          },
          {
            "name": "Counting Actor (scheduler=adaptive)",
            "value": 312.93225,
            "unit": "ms",
            "extra": "Target: Counting Actor\nTotal Iterations: 12\nThreads: 0\nScheduler: adaptive"
          },
          {
            "name": "Counting Actor (scheduler=GEDF_NP)",
            "value": 476.4121666666667,
            "unit": "ms",
            "extra": "Target: Counting Actor\nTotal Iterations: 12\nThreads: 0\nScheduler: GEDF_NP"
          },
          {
            "name": "Counting Actor (scheduler=NP)",
            "value": 306.5394166666667,
            "unit": "ms",
            "extra": "Target: Counting Actor\nTotal Iterations: 12\nThreads: 0\nScheduler: NP"
          },
          {
            "name": "Sleeping Barber (scheduler=adaptive)",
            "value": 93.84133333333334,
            "unit": "ms",
            "extra": "Target: Sleeping Barber\nTotal Iterations: 12\nThreads: 0\nScheduler: adaptive"
          },
          {
            "name": "Sleeping Barber (scheduler=GEDF_NP)",
            "value": 94.05566666666665,
            "unit": "ms",
            "extra": "Target: Sleeping Barber\nTotal Iterations: 12\nThreads: 0\nScheduler: GEDF_NP"
          },
          {
            "name": "Sleeping Barber (scheduler=NP)",
            "value": 95.56874999999998,
            "unit": "ms",
            "extra": "Target: Sleeping Barber\nTotal Iterations: 12\nThreads: 0\nScheduler: NP"
          },
          {
            "name": "Chameneos (scheduler=adaptive)",
            "value": 163.54333333333332,
            "unit": "ms",
            "extra": "Target: Chameneos\nTotal Iterations: 12\nThreads: 0\nScheduler: adaptive"
          },
          {
            "name": "Chameneos (scheduler=GEDF_NP)",
            "value": 318.91991666666667,
            "unit": "ms",
            "extra": "Target: Chameneos\nTotal Iterations: 12\nThreads: 0\nScheduler: GEDF_NP"
          },
          {
            "name": "Chameneos (scheduler=NP)",
            "value": 244.643,
            "unit": "ms",
            "extra": "Target: Chameneos\nTotal Iterations: 12\nThreads: 0\nScheduler: NP"
          },
          {
            "name": "Trapezoidal Approximation (scheduler=adaptive)",
            "value": 86.67416666666666,
            "unit": "ms",
            "extra": "Target: Trapezoidal Approximation\nTotal Iterations: 12\nThreads: 0\nScheduler: adaptive"
          },
          {
            "name": "Trapezoidal Approximation (scheduler=GEDF_NP)",
            "value": 87.31475,
            "unit": "ms",
            "extra": "Target: Trapezoidal Approximation\nTotal Iterations: 12\nThreads: 0\nScheduler: GEDF_NP"
          },
          {
            "name": "Trapezoidal Approximation (scheduler=NP)",
            "value": 88.36174999999999,
            "unit": "ms",
            "extra": "Target: Trapezoidal Approximation\nTotal Iterations: 12\nThreads: 0\nScheduler: NP"
          },
          {
            "name": "NQueens first N solutions (scheduler=adaptive)",
            "value": 352.54358333333334,
            "unit": "ms",
            "extra": "Target: NQueens first N solutions\nTotal Iterations: 12\nThreads: 0\nScheduler: adaptive"
          },
          {
            "name": "NQueens first N solutions (scheduler=GEDF_NP)",
            "value": 363.8135,
            "unit": "ms",
            "extra": "Target: NQueens first N solutions\nTotal Iterations: 12\nThreads: 0\nScheduler: GEDF_NP"
          },
          {
            "name": "NQueens first N solutions (scheduler=NP)",
            "value": 353.44958333333335,
            "unit": "ms",
            "extra": "Target: NQueens first N solutions\nTotal Iterations: 12\nThreads: 0\nScheduler: NP"
          },
          {
            "name": "Logistic Map Series (scheduler=adaptive)",
            "value": 60.31299999999999,
            "unit": "ms",
            "extra": "Target: Logistic Map Series\nTotal Iterations: 12\nThreads: 0\nScheduler: adaptive"
          },
          {
            "name": "Logistic Map Series (scheduler=GEDF_NP)",
            "value": 317.5824166666666,
            "unit": "ms",
            "extra": "Target: Logistic Map Series\nTotal Iterations: 12\nThreads: 0\nScheduler: GEDF_NP"
          },
          {
            "name": "Logistic Map Series (scheduler=NP)",
            "value": 197.50191666666663,
            "unit": "ms",
            "extra": "Target: Logistic Map Series\nTotal Iterations: 12\nThreads: 0\nScheduler: NP"
          },
          {
            "name": "Concurrent Sorted Linked List (scheduler=adaptive)",
            "value": 240.7775,
            "unit": "ms",
            "extra": "Target: Concurrent Sorted Linked List\nTotal Iterations: 12\nThreads: 0\nScheduler: adaptive"
          },
          {
            "name": "Concurrent Sorted Linked List (scheduler=GEDF_NP)",
            "value": 300.2460833333334,
            "unit": "ms",
            "extra": "Target: Concurrent Sorted Linked List\nTotal Iterations: 12\nThreads: 0\nScheduler: GEDF_NP"
          },
          {
            "name": "Concurrent Sorted Linked List (scheduler=NP)",
            "value": 283.24966666666666,
            "unit": "ms",
            "extra": "Target: Concurrent Sorted Linked List\nTotal Iterations: 12\nThreads: 0\nScheduler: NP"
          },
          {
            "name": "Fork Join (throughput) (scheduler=adaptive)",
            "value": 36.23325,
            "unit": "ms",
            "extra": "Target: Fork Join (throughput)\nTotal Iterations: 12\nThreads: 0\nScheduler: adaptive"
          },
          {
            "name": "Fork Join (throughput) (scheduler=GEDF_NP)",
            "value": 156.83533333333335,
            "unit": "ms",
            "extra": "Target: Fork Join (throughput)\nTotal Iterations: 12\nThreads: 0\nScheduler: GEDF_NP"
          },
          {
            "name": "Fork Join (throughput) (scheduler=NP)",
            "value": 71.2965,
            "unit": "ms",
            "extra": "Target: Fork Join (throughput)\nTotal Iterations: 12\nThreads: 0\nScheduler: NP"
          },
          {
            "name": "Radix Sort (scheduler=adaptive)",
            "value": 643.6772500000001,
            "unit": "ms",
            "extra": "Target: Radix Sort\nTotal Iterations: 12\nThreads: 0\nScheduler: adaptive"
          },
          {
            "name": "Radix Sort (scheduler=GEDF_NP)",
            "value": 688.6944166666667,
            "unit": "ms",
            "extra": "Target: Radix Sort\nTotal Iterations: 12\nThreads: 0\nScheduler: GEDF_NP"
          },
          {
            "name": "Radix Sort (scheduler=NP)",
            "value": 470.34066666666666,
            "unit": "ms",
            "extra": "Target: Radix Sort\nTotal Iterations: 12\nThreads: 0\nScheduler: NP"
          },
          {
            "name": "Big (scheduler=adaptive)",
            "value": 244.8213333333333,
            "unit": "ms",
            "extra": "Target: Big\nTotal Iterations: 12\nThreads: 0\nScheduler: adaptive"
          },
          {
            "name": "Big (scheduler=GEDF_NP)",
            "value": 256.15866666666665,
            "unit": "ms",
            "extra": "Target: Big\nTotal Iterations: 12\nThreads: 0\nScheduler: GEDF_NP"
          },
          {
            "name": "Big (scheduler=NP)",
            "value": 236.52391666666668,
            "unit": "ms",
            "extra": "Target: Big\nTotal Iterations: 12\nThreads: 0\nScheduler: NP"
          },
          {
            "name": "All-Pairs Shortest Path (scheduler=adaptive)",
            "value": 174.63441666666665,
            "unit": "ms",
            "extra": "Target: All-Pairs Shortest Path\nTotal Iterations: 12\nThreads: 0\nScheduler: adaptive"
          },
          {
            "name": "All-Pairs Shortest Path (scheduler=GEDF_NP)",
            "value": 168.877,
            "unit": "ms",
            "extra": "Target: All-Pairs Shortest Path\nTotal Iterations: 12\nThreads: 0\nScheduler: GEDF_NP"
          },
          {
            "name": "All-Pairs Shortest Path (scheduler=NP)",
            "value": 169.81766666666667,
            "unit": "ms",
            "extra": "Target: All-Pairs Shortest Path\nTotal Iterations: 12\nThreads: 0\nScheduler: NP"
          }
        ]
      }
    ],
    "Lingua Franca C Benchmark -- Single-Threaded": [
      {
        "commit": {
          "author": {
            "name": "lf-lang",
            "username": "lf-lang"
          },
          "committer": {
            "name": "lf-lang",
            "username": "lf-lang"
          },
          "id": "62c26925b27c32189fbba37f542c5a6f9f01389d",
          "message": "Continuous benchmarking",
          "timestamp": "2022-06-08T21:36:19Z",
          "url": "https://github.com/lf-lang/reactor-c/pull/51/commits/62c26925b27c32189fbba37f542c5a6f9f01389d"
        },
        "date": 1659052035233,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "Logistic Map Series",
            "value": 48.73575,
            "unit": "ms",
            "extra": "Target: Logistic Map Series\nTotal Iterations: 12\nThreads: 2"
          },
          {
            "name": "Sleeping Barber",
            "value": 41.383833333333335,
            "unit": "ms",
            "extra": "Target: Sleeping Barber\nTotal Iterations: 12\nThreads: 2"
          },
          {
            "name": "Trapezoidal Approximation",
            "value": 177.1481666666667,
            "unit": "ms",
            "extra": "Target: Trapezoidal Approximation\nTotal Iterations: 12\nThreads: 2"
          },
          {
            "name": "Ping Pong",
            "value": 46.73491666666666,
            "unit": "ms",
            "extra": "Target: Ping Pong\nTotal Iterations: 12\nThreads: 2"
          },
          {
            "name": "Fork Join (throughput)",
            "value": 22.77525,
            "unit": "ms",
            "extra": "Target: Fork Join (throughput)\nTotal Iterations: 12\nThreads: 2"
          },
          {
            "name": "Concurrent Dictionary",
            "value": 79.39883333333333,
            "unit": "ms",
            "extra": "Target: Concurrent Dictionary\nTotal Iterations: 12\nThreads: 2"
          },
          {
            "name": "Thread Ring",
            "value": 40.41425,
            "unit": "ms",
            "extra": "Target: Thread Ring\nTotal Iterations: 12\nThreads: 2"
          },
          {
            "name": "Radix Sort",
            "value": 165.22958333333335,
            "unit": "ms",
            "extra": "Target: Radix Sort\nTotal Iterations: 12\nThreads: 2"
          },
          {
            "name": "A-Star Search",
            "value": 38.2,
            "unit": "ms",
            "extra": "Target: A-Star Search\nTotal Iterations: 12\nThreads: 2"
          },
          {
            "name": "Precise Pi Computation",
            "value": 161.35066666666668,
            "unit": "ms",
            "extra": "Target: Precise Pi Computation\nTotal Iterations: 12\nThreads: 2"
          },
          {
            "name": "Recursive Matrix Multiplication",
            "value": 219.28716666666665,
            "unit": "ms",
            "extra": "Target: Recursive Matrix Multiplication\nTotal Iterations: 12\nThreads: 2"
          },
          {
            "name": "Producer Consumer (bounded)",
            "value": 270.4241666666666,
            "unit": "ms",
            "extra": "Target: Producer Consumer (bounded)\nTotal Iterations: 12\nThreads: 2"
          },
          {
            "name": "Big",
            "value": 174.39408333333336,
            "unit": "ms",
            "extra": "Target: Big\nTotal Iterations: 12\nThreads: 2"
          },
          {
            "name": "NQueens first N solutions",
            "value": 658.509,
            "unit": "ms",
            "extra": "Target: NQueens first N solutions\nTotal Iterations: 12\nThreads: 2"
          },
          {
            "name": "Cigarette Smokers",
            "value": 1.0403333333333336,
            "unit": "ms",
            "extra": "Target: Cigarette Smokers\nTotal Iterations: 12\nThreads: 2"
          },
          {
            "name": "Concurrent Sorted Linked List",
            "value": 240.09900000000002,
            "unit": "ms",
            "extra": "Target: Concurrent Sorted Linked List\nTotal Iterations: 12\nThreads: 2"
          },
          {
            "name": "Bank Transaction",
            "value": 206.39824999999996,
            "unit": "ms",
            "extra": "Target: Bank Transaction\nTotal Iterations: 12\nThreads: 2"
          },
          {
            "name": "Philosophers",
            "value": 44.07183333333333,
            "unit": "ms",
            "extra": "Target: Philosophers\nTotal Iterations: 12\nThreads: 2"
          },
          {
            "name": "All-Pairs Shortest Path",
            "value": 306.26275,
            "unit": "ms",
            "extra": "Target: All-Pairs Shortest Path\nTotal Iterations: 12\nThreads: 2"
          },
          {
            "name": "Chameneos",
            "value": 115.46725000000002,
            "unit": "ms",
            "extra": "Target: Chameneos\nTotal Iterations: 12\nThreads: 2"
          },
          {
            "name": "Filter Bank",
            "value": 215.36475,
            "unit": "ms",
            "extra": "Target: Filter Bank\nTotal Iterations: 12\nThreads: 2"
          },
          {
            "name": "Counting Actor",
            "value": 125.11158333333333,
            "unit": "ms",
            "extra": "Target: Counting Actor\nTotal Iterations: 12\nThreads: 2"
          }
        ]
      }
    ]
  }
}