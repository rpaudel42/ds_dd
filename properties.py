class RabbitConnection:
    hostName = "127.0.0.1"
    queueName = "graph_stream"
    routingKey = "graph_stream"
    heartbeat = 0

class SubGen:
    s1_positive_sample = "subgen/pos"
    s1_negative_sample = "subgen/neg"
    s2_positive_sample = "subgen/s2pos"
    s2_negative_sample = "subgen/s2neg"
    run_command = "subgen/subgen"

class GBAD:
    gbad_home = "gbad"
    graph_folder = "gbad/graphs"
    subgraph = "subgraph.g"
    graph_file = "G.g"
    positive_graph = "pos_G.g"
    negative_graph = "neg_G.g"
    minsize = 1
    maxsize = 10
    run_command = "bin/gbad " + " -minsize " + str(minsize) + " -maxsize " + str(maxsize)
    algorithm = "mdl"

class SubDue:
    SubDueFolder ="subdue"
    graphFolder = "subdue/graphs"
    subgraph = "subgraph.g"
    graphFile = "G.g"
    minsize = 2
    maxsize = 10
    nsubs = 5
    beam = 4
    subdueCommand = "bin/subdue "+ " -minsize "+ str(minsize) +" -maxsize " + str(maxsize) + " -beam " + str(beam) +" -nsubs "+ str(nsubs)
    #subdueCommand = "bin/subdue "  + " -nsubs " + str(nsubs)


class Experiment:
    iterations = 50
    param_n = 10        #Number of subgraph
    param_w = 50        #Window Size
    param = [param_n, param_w]


class RULSIF:
    n = 50
    k = 10
    alpha = 0.1
    k_fold = 5
    th = 4.4

class DataList:
    # datasetname = (<dataset name>, <total graph>, <expected drift points>, <graph file name>, <isSyntetic (True or False)> , <param>(parameter array))
    SD1 = ('SD1', 2000, [1000], 'dataset/SD1.g', True, Experiment.param)
    SD2 = ('SD2', 6000, [1001, 2001, 3001, 4001, 5001], 'dataset/SD2.g', False, Experiment.param)
    DBLP = ('DBLP', 2000, [1000], 'dataset/DBLP.g', False, Experiment.param)
    AIDS = ('AIDS', 2000, [1600], 'dataset/aids.g', False, Experiment.param)
    DOS = ('DOS', 3201, [2980], 'dataset/DOS.g', False, Experiment.param)
    MUTA = ('MUTA', 4337, [2402], 'dataset/muta.g', False, Experiment.param)

    #make a list of all datasets
    data_list = [DBLP]
