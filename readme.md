**Requirements**
=============

1. python (3.0)
2. Python packages:
	- networkx
	- scipy
	- pylab
	- numpy


**Usage**
======

$ make

if Make is not installed
------------------------
$ python3 main.py


**Notes**
=====

1. Make sure "dataset" folder have the associated data files
2. Make sure you have "gbad" and "subgen" folder and the associated files
3. To run the program, run following commands:
    -make clean
    -make
    -make install
4. This code implements following paper:
    Paudel R and Eberle W. An Approach For Concept Drift Detection in a Graph Stream Using Discriminative Subgraphs. (2019 March)

**Description**
n this paper, we propose an unsupervised approach for concept drift detection in graph stream using discriminative subgraphs.
The main idea is to process the graph stream using a sliding window technique and decomposing each graph into a number
of discriminative subgraphs that best compresses the graph using an MDL heuristic.
We then compute the entropy of the window based on the distribution of discriminative subgraph, w.r.t. the graphs,
by moving one step forward in the sliding window. The well known direct density-ratio estimation approach called
Relative UnconstrainedLeast-Squares Importance Fitting (RuLSIF) is employed for detecting concept drift
in the entropy series.
![Discriminative subgraphs-based Drift Detection Algorithm](http://rpaudel42.github.io/assets/dsdd.png)
<br/>
The result of DSDD and other baseline approach in 4 different dataset is shown below:
![Results](http://rpaudel42.github.io/assets/dsdd_result.png)
If you have further inquiry please email at rpaudel42@students.tntech.edu