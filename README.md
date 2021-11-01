# Concurrent Binary Search Tree

## SUMMARY
We want to implement 3 - 4 concurrent version of binary search tree (BST) data structures. The baseline version will be BST protected by single coarse-grained lock. The second version will be a fine-grained lock protected BST. The third version will be a lock free version BST. If we can get access to a machine supporting transactional memory and we have time, we will also implement a transectional memory version BST.
After implementation, we will do experiments and measure the performance of each version on different number of concurrent threads, or with different ratio of three operations (insert, delete and find), or in different operation sequence (e.g. all finds occur after inserts or finds and inserts interleaving with each other). We want to analyze the pros and cons of each implementation, and also compare their performance.

## BACKGROUND

## THE CHALLENGE

## RESOURCES

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