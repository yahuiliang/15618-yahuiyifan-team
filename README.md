# Concurrent Binary Search Tree
Here is the PDF version of [project proposal](./Project_Proposal.pdf).
Here is the PDF version of [milestone report](./Milestone_Report.pdf).

## SUMMARY
We want to implement 3 - 4 concurrent version of binary search tree (BST) data structures, including BST protected by single coarse-grained lock, fine-grained lock protected BST, lock free version BST and a transactional memory version BST if we have time. We will do experiments and measure the performance of each version on different workloads and concurrent thread count to analyze the pros and cons of each implementation and compare their performance.

## BACKGROUND
A binary search tree (BST) is a data structure which allows data to be searched, find, and delete with a time complexity O(log(n)). In a BST, the most important property is that a parent node will always have key larger than the key of its left sub-tree and smaller than the key of its right sub-tree.  Three operations are supported in BST, insertion, deletion, and searching. When inserting, the location where the new node will be inserted need to guarantee that the property of BST still holds. After deletion, the BST may need to be adjusted to make sure the property still holds. In BST, data can be stored with hierarchies so that the modification to one sub-tree would not affect other sub-trees, though the structure of current sub-tree may be changed.

Binary search trees have lots of applications. Sorting can be performed using a binary search tree by inserting all elements and then perform in-order traversal. Another important application is database, which can use B-tree to sort data. Although not exactly using BST, B-tree is just a generalization of BST allowing each parent node having more than two child nodes. We want to implement concurrent BST because as long as BST supports concurrency, the applications of BST can explore parallelism, so the potential benefits can be large. We want to compare different implementations of concurrent BST to know what is the most efficient implementation or which implementation is the most suitable for a certain workload pattern so that the applications of BST can choose the best implementation to speedup more when parallelized.

## THE CHALLENGE
One of the most important problems to be solved when using concurrent data structures is how to ensure the data integrity when multiple threads are trying to access or modify the same part of the data structure. Solving this problem can be challenging in binary search tree since deleting a node will not only impact the node itself, its parent node and its children node. The structure of the sub-tree containing that node may be affected since in binary search tree since we need to maintain the correct order between a parent node and its left and right child node. When using different methods to ensure the data integrity, efficiency is also needed to be considered and can be another challenging part. The overhead of operations used to maintain data integrity must not over-shade the speedup brought by parallelization. Potential problems brought by locks and concurrency, such as deadlock, must be dealt with, which may also be challenging.

Another possible challenging part in our project is how to generate different workloads for experiments. It is likely that a certain version of concurrent BST perform better than other versions when using a particular workload pattern (e.g. more inserts than deletes). If we want to explore the pros and cons of each version, we may need to come up with a few workload patterns, which may be challenging.

## RESOURCES
We found three papers relate to fine-grained lock protected BST, lock free version BST, and transaction memory BST.

Kung, H. T., & Lehman, P. L. (1979). Concurrent manipulation of binary search trees . Carnegie-Mellon University, Dept. of Computer Science.


Natarajan, A., & Mittal, N. (2014). Fast concurrent lock-free binary search trees. SIGPLAN Notices, 49(8), 317–328. https://doi.org/10.1145/2692916.2555256


Bronson, N. G., Casper, J., Chafi, H., & Olukotun, K. (2010). A practical concurrent binary search tree. SIGPLAN Notices, 45(5), 257–268. https://doi.org/10.1145/1837853.1693488

We will use GHC machines and PSC machine. We plan to implement different versions of BST and verify their correctness on GHC machines. To test their performance when using different number of threads, we may need to use PSC machine as it has more cores. Tests on how different workload affects the performance of different versions of BST will probably be carried on GHC machines as well.

## GOALS AND DELIVERABLES
### Plan to achieve
- Implement the tree with coarse-grained lock supported
- Implement the tree with fine-grained lock supported
- Implement the tree without locking mechanism
- Implement the tree with software transaction memory supported
- Generate different insertion and deletion workloads
- Verify the correctness of three implementations
- Carry out experiments on different implementations of BST with different thread count and workloads
- Analyze performance, pros and cons of each BST implementation

In order to verify the correctness, we want our data to be consistent before and after a group of manipulations. Also, we do not want deadlocks appear in our implementation, we would have a timeout mechanism to detect whether deadlock happens. All details about these concurrent trees have been specified in papers that we found. Therefore, we are confident that we can implement them.

### Hope to achieve
If we are on the right track, and finish above features earlier, we would like to add

- fine-grained tree balancing
- lock-free tree balancing
- STM tree balancing

However, if we did not have enough time to implement all 4 trees, we want first 3 of 4 tree implementations to be done, i.e. BST protected by coarse-grained lock, fine-grained lock, and lock-free BST.

### What to show
We plan to show several speedup graphs during our post session. The speedup graphs will be about Speedup of different BST implementations vs. thread count when using different workloads. There will be one graph for each workload.

These speedup graphs can help people know which BST performs best under different scenarios.

### Expectations
Either fine-grained concurrent BST, lock-free BST, and STM BST implementations should perform better in speedup than coarse-grained concurrent BST, as well as the sequential version of BST manipulations. Different insertion, deletion, searching workloads may lead to different implementations outperforming others.

## PLATFORM CHOICE
We are going to use C++ to implement our data structures. Our team members are more experienced with using C/C++ rather than some high-level languages like Java, Python, and etc. Also, C++ allows us to manage memory on our own, and this is usefull when the paper explicitly specifies an garbage collection mechanism.

## SCHEDULE
| Week | Date | Tasks | Done |
| --- | --- | :-: | :-: |
| 1 | 11.1 - 11.7 | Proposal <br> Literature review <br> Coarse-grained lock version BST | Yes |
| 2 | 11.8 - 11.14 | Fine-grained lock version BST <br> Lock-free version BST | Fine-grained version finished (Memory release problem not solved) <br> Lock-free version started |
| 3 | 11.15 - 11.21 | Lock-free version BST <br> Transactional memory version BST <br> Milestone report | Lock-free version started <br> Transactional memory version not started yet |
| 4 | 11.22 - 11.28 | Transactional memory version BST <br> Experiment workload generation | No |
| 5 | 11.29 - 12.5 | Experiments and analysis | No |
| 6 | 12.6 - 12.10 | Final report <br> Poster | No |

## Checkpoint
### Schedule
We are behind schedule for the time being.  Part of the reason is that we just had exam, and we did not take itinto consideration when making the schedule.  However, the main reason is that we met the problem of safe releaseof memory.  When erasing a node, we cannot immediately release the memory of the node because there may beother threads accessing the memory.  The paper we referenced did not describe how to do garbage collection indetail.  Locking the whole tree and do garbage collection periodically will hurt performance too much.  We alsotried using smart shared pointer in C++, but it is too expensive, and thus it also hurts the performance a lot.We decided to implement the hazard pointer method, which will be implemented earlier next week.  We adjustedour schedule, and implementing the transactional memory version of BST will be a depending task depending onour future progress.

### Summary
We have finished literature review about fine-grained, lock-free, and transactional memory versions of BST. Wehave implemented the coarse-grained lock version BST. It is based on the simple single-thread BST and addinga lock for the whole tree.  We have implemented the fine-grained lock version BST. It is mainly based on theidea  of  hand-over-hand  lock.   For  the  erase  operation,  the  paper  we  referenced  adopted  the  method  to  rotatethe node to be erased to leaf position if it is not at the first place so that the tree may be more balanced andbenefit future operations on tree.  We have finished implementing the basic operations while releasing memoryonly when destructing the tree due to the safe memory release problem caused by multiple threads.  However,this implementation cannot scale to support 100,000 erases.  Therefore, we have been seeking ways to resolve thesafe memory release problem, and decided to implement hazard pointer.

### Goal and Deliverable
#### Plan to achieve
- Implement the tree with coarse-grained lock supported
- Implement the tree with fine-grained lock supported
- Implement the tree without locking mechanism
- Generate different insertion and deletion workloads
- Verify the correctness of three implementations
- Carry out experiments on different implementations of BST with different thread count and workloads
- Analyze performance, pros and cons of each BST implementation

#### Hope to achieve
- Transactional memory based concurrent tree implementation
- Fine-grained tree balancing
- Lock-free tree balancing

We have reached the first goal and almost reached the second goal.  We also achieved the fifth goal for coarse-grained lock and current fine-grained lock versions of BST. We moved the goal to implement the transactionalmemory version BST to hope to achieve goal since we spent more time on solving the safe memory release problem.We also achieved the fine-grained tree balancing goal in the hope to achieve goals to some extent.  The algorithmof erasing we chose for fine-grained tree is to keep rotating the tree until the node to be erased is a leaf node.Rotating will likely balance the tree, although the balancing may not be perfect since we are not rotating basedon height difference.

#### What to show
We plan to show several speedup graphs during our post session.  The speedup graphs will be about Speedupof different BST implementations vs.  thread count when using different workloads.  There will be one graph foreach workload.  These speedup graphs can help people know which BST performs best under different scenarios.

### Preliminary Results
When deciding whether we can use C++ smart shared pointer to resolve the safe memory release problem, we tested time needed for the fine-grained lock BST implemented by shared pointer to insert and search 100,000 different elements, and the result is 2.42 seconds. We then tested time needed for the coarse-grained lock BST to insert and search 100,000 different elements, and the result is 1.542 seconds. Therefore, we found out that shared pointers are too expensive to use and will hurt the performance so bad that the fine-grained lock BST will even be slower than the coarse-grained lock BST. We think the reason is that shared pointer objects will need to maintain the reference count of the memory location. When different threads are accessing the same memory location, the reference count will be read and written by different threads, causing frequent invalidations to maintain cache coherence and much internal communication traffic, so the overhead of using shared pointers is very high. Therefore, we decided that we cannot use shared pointer, but will implement the hazard pointer method.

### Concerns
The main concern we have now is how to implement hazard pointer efficiently. The hazard pointer will be used both in fine-grained lock BST and lock-free BST, and after implementing it, we think we can adjust fine-grained lock BST and implement lock-free BST quite quickly.