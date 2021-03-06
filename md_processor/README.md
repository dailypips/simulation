# Market Data Simulation

Configurable tool to test raw performance and alternate components of market data systems

## Overview

This part simulation frame work is designed to enable an assortment of pluggable choices of stream adapters for file, message queue or packet capture. Different stream formats - binary, csv , json, fast fix etc. We can examine different order book models from trading or even betting system such as betafair. Also you can change the underlying order book data structure and select different outbound sinks. This is all targeted at being able to measure performance and reliability over differnt combinations of processing mechanisms and data models.

The following diagtram shows a pluggable desgin consisteng of a (MD) market data adapter/translator taking the raw feed and tranforming into a stream  messages. These are given to the (MD Processor) processor whose task is to create order book and trading messages which feed to consumer systems for publishing, logging, alogorithmic processing or trading information systems etc via high speed message queue.

<figure role="group">
<img src="images/MDProcessorOverview.jpg" alt="MDProcessor" style="width: 600px; height: 300px;" title="MDProcessor"/>
<figcaption>Fig 1. Overview</figcaption>
</figure>

## Processor
The Processor design is aimed at handling data produced by the md_simulator or real world market data producers like betfair or other trading platforms. Processing this data enables different simulation strategies from different sources such as betting or trading to create output with trade and pricing book structure in a form that can be loaded into analysis tools.

The processor is resonsible for -

1. Parsing the json events and trap any corruption error type scenarios.
2. Create event objects which can be presented to the order book
3. Order book implement the methods that respond to add, modify, cancel and trade
4. Back the order book which a storage data structure which will allow prices at levels to be maintained and orders to be held at each price. Levels are Buy(Bid) or Sell(Ask).
5. The order book needs to present itself in a readable form so implement some sort of output stream/print function
6. The handler then is created to make use of the event objects and direct the methods to call on the order book object. The interface look and feel tries to keep in line with the stub code given in specification.
7. Processor provides a methods to print the error summary from the processing of the feed and this in turn implies we need some sort of globally available statistics recording object as both the order book and parsing parts of the flow.


<figure role="group">
<img src="images/MDProcessorDetail.jpg" alt="MDProcessor" style="width: 800px; height: 500px;" title="MDProcessor"/>
<figcaption>Fig 2. MDProcessor</figcaption>
</figure>

---
## Installation

You can install md_processor (if you have not already done so for md_simulation) from `github` using the 

```{bash}
cd <a repository directory>
git clone https://github.com/cdtait/simulation.git
```

The c++ code is based on c++11 and was developed with g++ 4.8.2 and stl. Included 3rd party json_spirit has dependancy on boost::spirit. 

However src directory has is all the c++ files required for the project, the main file is in md.cpp. Four further sub directories are included. These too are open source inlcudes to suport the md processor. 

* The disruptor is a c++ version of the LMAX disruptor. 
* The vectorclass directory contains VCL which implements vectorization libraries for vector and matrix arithmetic on SSE2 and AVX supported platforms. 
* The strtk dirctory has some useful templates to support parsing string streams into numbers and avoids boost lexical_cast, stringstream and atoi/f calls in favour of more optimal type creation.
* The json_spirit implements the json parser we use for the input straem

```{bash}
cd  simulation/md_processor
```

This is actually a eclipse project so it can be imported in eclipse if desired. At this level there a Debug,Release and src directories. To compile a release version and to make a release build. Change directory into Release and type 

```{bash}
$ cd Release
$ make clean
rm -rf  ./src/md.o  ./src/md.d  md_processor

$ make
Building file: ../src/md.cpp
Invoking: GCC C++ Compiler
g++ -I"../src" -I"../src/vectorclass" -O3 -Wall -c -fmessage-length=0 -std=c++11 -fabi-version=4 -MMD -MP -MF"src/md.d" -MT"src/md.d" -o "src/md.o" "../src/md.cpp"
Finished building: ../src/md.cpp
 
Building target: md_processor
Invoking: GCC C++ Linker
g++  -o "md_processor"  ./src/md.o   -lpthread
Finished building target: md_process
```
It should only take a few seconds.

## Ipython notebook

As an option there is a sample of data which can be used with the ipython notebook md_example.ipynb however the data director samples are compressed

```{bash}
gunzip data/*.gz
```
Now run the notebook

```{bash}
$ ipython notebook
```
Now select md_example.ipynb from <a repository directory>/simulation/md_processor


### Market Data Example

Here we are taking random walk data generated from the md_simulator package which has then been
converted to csv format from the md_processor. So whart we are doing is extracting the price where trades occur and showing which are buys and sells around the price movements.

#### Quick Notebook example of using panadas and ploting

Load the data from the csv and plot mid price with buy/sell trades
```python
    %matplotlib inline

    import pandas as pd
    import matplotlib.pyplot as plt
    import numpy as np
    
    pd.options.display.max_columns=40
    pd.options.display.max_rows=1000

    path = 'data/md-test-2.C-M.csv'
    book_data = pd.read_csv(path, index_col=False,header=None,nrows=50000,skiprows=10)

    ask_trade_prices = book_data[book_data.iloc[:,1]=='S'].iloc[:,3]
    bid_trade_prices = book_data[book_data.iloc[:,1]=='B'].iloc[:,3]

    book=book_data[book_data.iloc[:,1]=='U']
    book['mid_price']=(book.iloc[:,6]+book.values[:,7])/2.0

    plt.figure(figsize=(10,5))
    book['mid_price'].plot(color='g')
    plt.scatter(ask_trade_prices.index,ask_trade_prices.values,color='r',s=5)
    plt.scatter(bid_trade_prices.index,bid_trade_prices.values,color='b',s=5)


    <matplotlib.collections.PathCollection at 0x7677210>
```

![png](images/md_example_8_1.png)


## Test run

The following is an example run of the md_processor.

```{bash}
$ Release/md_processor -p C -d M -f data/md-test-2.json 2> data/md-test.C-M.stats > data/md-test-2.C-M.csv
```

This would produce the md-test-2.C-M.csv which can be used later for analysis. The md-test.C-M.stats are a record of
errors that if the source data/md-test-2.json was produced with deliberate or otherwise invalid data these would be 
recorded in the stats.
```{bash}
$ Release/md_processor ?
Usage: md_processor -f <file name> [-p T|C] [-d M|H|V] [-x L] [-t A|S|C] [-a F|P|ZR|ZP|ZS ] [-s P|D|N ]                    
       -f is name of file to stream the input
         The file name can be relative or absolute
       -p is for the type of print out put you wish to see
         T is a text book and C is a csv format output
       -d selects the underlying book structure to test
         M is a map, H is a hash and V is vector base data structures
       -x select the type of parser model to test
         L is the simple token list parse for csv text or json line formats
       -t select the type of token container use in test
         A is a json_spirit based array, S is a strtk string vector and C is a custom char* vector
         Important - When using json_spirit it will expect a json file where S and C expect csv.
       -a select the adapter to source the market data
         F is a file based input
         Zx uses a zeromq broken into different models aimed at demonstrating messaging
         ZR - Reliable-request-reply
         ZS - Subscriber
         ZP - Pull
         P is a pcap device either file or ethernet alias
       -s select publisher to handle the output we want
         P is print based publisher
         D is the disruptor
         N is none action publisher
```

 * M is a std::map based order book
 * H is a std::unordered_map
 * V is std::vector base map


### Credits
- [disruptor](https://github.com/fsaintjacques/disruptor--) by François Saint-Jacques
- [json_spirit](http://www.codeproject.com/Articles/20027/JSON-Spirit-A-C-JSON-Parser-Generator-Implemented) by John W. Wilkinson
- [strtk](http://www.partow.net/programming/strtk/index.html) by Arash Partow
- [vcl](www.agner.org/optimize) by Agner Fog

### License

md_processor is licensed under the GPLv2 License. 


