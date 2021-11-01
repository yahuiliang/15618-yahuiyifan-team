# Concurrent Binary Search Tree

## SUMMARY
We want to implement 3 - 4 concurrent version of binary search tree (BST) data structures. The baseline version will be BST protected by single coarse-grained lock. The second version will be a fine-grained lock protected BST. The third version will be a lock free version BST. If we can get access to a machine supporting transactional memory and we have time, we will also implement a transectional memory version BST.
After implementation, we will do experiments and measure the performance of each version on different number of concurrent threads, or with different ratio of three operations (insert, delete and find), or in different operation sequence (e.g. all finds occur after inserts or finds and inserts interleaving with each other). We want to analyze the pros and cons of each implementation, and also compare their performance.

## BACKGROUND

## THE CHALLENGE

## RESOURCES
Kung, H. T., & Lehman, P. L. (1979). Concurrent manipulation of binary search trees . Carnegie-Mellon University, Dept. of Computer Science.


Natarajan, A., & Mittal, N. (2014). Fast concurrent lock-free binary search trees. SIGPLAN Notices, 49(8), 317–328. https://doi.org/10.1145/2692916.2555256


Chatterjee, B., Nguyen, N., & Tsigas, P. (2014). Efficient Lock-free Binary Search Trees.


Bronson, N. G., Casper, J., Chafi, H., & Olukotun, K. (2010). A practical concurrent binary search tree. SIGPLAN Notices, 45(5), 257–268. https://doi.org/10.1145/1837853.1693488

## GOALS AND DELIVERABLES

## PLATFORM CHOICE

## SCHEDULE
| Date      | Goal |
| ----------- | ----------- |
| Nov 2 | Read paper of Concurrent Manipulation of Binary Search Trees(lock const nodes) |
| Nov 3 11:59pm | Project Proposal Due |
| Nov 5 | Read other 3 papers |
| Nov 10 | Concurrent Manipulation of Binary Search Trees(lock const nodes) algorithm implementation |
| Nov 17 | Fast Concurrent Lock-Free Binary Search Trees implementation |
| Nov 22, 9:00am | Milestone due |
| Nov 24 | Efficient Lock-free Binary Search Trees implementation |
| Nov 28 | Data pattern generator |
| Dec 1 | Simulation results with different concurrent tree versions |
| Dec 9 11:59pm | Final report due |
| Dec 10 5:30-8:30pm | Poster Session |