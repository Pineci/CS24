# -m 31440

# output 72 bytes in use; 3 refs in use
mem()
chain = [None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None]
# output [None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None, None]
print(chain)
# output 544 bytes in use; 5 refs in use
mem()
i = 1
while i < len(chain):
    next = [1, 2, 3]
    next[i % len(next)] = chain[i - 1]
    chain[i] = next
    del next
    i = i + 1
del i
# output [None, [1, None, 3], [1, 2, [1, None, 3]], [[1, 2, [..., ..., ...]], 2, 3], [1, [[..., ..., ...], 2, 3], 3], [1, 2, [1, [..., ..., ...], 3]], [[1, 2, [..., ..., ...]], 2, 3], [1, [[..., ..., ...], 2, 3], 3], [1, 2, [1, [..., ..., ...], 3]], [[1, 2, [..., ..., ...]], 2, 3], [1, [[..., ..., ...], 2, 3], 3], [1, 2, [1, [..., ..., ...], 3]], [[1, 2, [..., ..., ...]], 2, 3], [1, [[..., ..., ...], 2, 3], 3], [1, 2, [1, [..., ..., ...], 3]], [[1, 2, [..., ..., ...]], 2, 3], [1, [[..., ..., ...], 2, 3], 3], [1, 2, [1, [..., ..., ...], 3]], [[1, 2, [..., ..., ...]], 2, 3], [1, [[..., ..., ...], 2, 3], 3], [1, 2, [1, [..., ..., ...], 3]], [[1, 2, [..., ..., ...]], 2, 3], [1, [[..., ..., ...], 2, 3], 3], [1, 2, [1, [..., ..., ...], 3]], [[1, 2, [..., ..., ...]], 2, 3], [1, [[..., ..., ...], 2, 3], 3], [1, 2, [1, [..., ..., ...], 3]], [[1, 2, [..., ..., ...]], 2, 3], [1, [[..., ..., ...], 2, 3], 3], [1, 2, [1, [..., ..., ...], 3]], [[1, 2, [..., ..., ...]], 2, 3], [1, [[..., ..., ...], 2, 3], 3], [1, 2, [1, [..., ..., ...], 3]], [[1, 2, [..., ..., ...]], 2, 3], [1, [[..., ..., ...], 2, 3], 3], [1, 2, [1, [..., ..., ...], 3]], [[1, 2, [..., ..., ...]], 2, 3], [1, [[..., ..., ...], 2, 3], 3], [1, 2, [1, [..., ..., ...], 3]], [[1, 2, [..., ..., ...]], 2, 3], [1, [[..., ..., ...], 2, 3], 3], [1, 2, [1, [..., ..., ...], 3]], [[1, 2, [..., ..., ...]], 2, 3], [1, [[..., ..., ...], 2, 3], 3], [1, 2, [1, [..., ..., ...], 3]], [[1, 2, [..., ..., ...]], 2, 3], [1, [[..., ..., ...], 2, 3], 3], [1, 2, [1, [..., ..., ...], 3]], [[1, 2, [..., ..., ...]], 2, 3], [1, [[..., ..., ...], 2, 3], 3], [1, 2, [1, [..., ..., ...], 3]], [[1, 2, [..., ..., ...]], 2, 3], [1, [[..., ..., ...], 2, 3], 3], [1, 2, [1, [..., ..., ...], 3]], [[1, 2, [..., ..., ...]], 2, 3], [1, [[..., ..., ...], 2, 3], 3], [1, 2, [1, [..., ..., ...], 3]], [[1, 2, [..., ..., ...]], 2, 3], [1, [[..., ..., ...], 2, 3], 3], [1, 2, [1, [..., ..., ...], 3]], [[1, 2, [..., ..., ...]], 2, 3], [1, [[..., ..., ...], 2, 3], 3], [1, 2, [1, [..., ..., ...], 3]], [[1, 2, [..., ..., ...]], 2, 3], [1, [[..., ..., ...], 2, 3], 3], [1, 2, [1, [..., ..., ...], 3]], [[1, 2, [..., ..., ...]], 2, 3], [1, [[..., ..., ...], 2, 3], 3], [1, 2, [1, [..., ..., ...], 3]], [[1, 2, [..., ..., ...]], 2, 3], [1, [[..., ..., ...], 2, 3], 3], [1, 2, [1, [..., ..., ...], 3]], [[1, 2, [..., ..., ...]], 2, 3], [1, [[..., ..., ...], 2, 3], 3], [1, 2, [1, [..., ..., ...], 3]], [[1, 2, [..., ..., ...]], 2, 3], [1, [[..., ..., ...], 2, 3], 3], [1, 2, [1, [..., ..., ...], 3]], [[1, 2, [..., ..., ...]], 2, 3], [1, [[..., ..., ...], 2, 3], 3], [1, 2, [1, [..., ..., ...], 3]], [[1, 2, [..., ..., ...]], 2, 3], [1, [[..., ..., ...], 2, 3], 3], [1, 2, [1, [..., ..., ...], 3]], [[1, 2, [..., ..., ...]], 2, 3], [1, [[..., ..., ...], 2, 3], 3], [1, 2, [1, [..., ..., ...], 3]], [[1, 2, [..., ..., ...]], 2, 3], [1, [[..., ..., ...], 2, 3], 3], [1, 2, [1, [..., ..., ...], 3]], [[1, 2, [..., ..., ...]], 2, 3], [1, [[..., ..., ...], 2, 3], 3], [1, 2, [1, [..., ..., ...], 3]], [[1, 2, [..., ..., ...]], 2, 3], [1, [[..., ..., ...], 2, 3], 3], [1, 2, [1, [..., ..., ...], 3]], [[1, 2, [..., ..., ...]], 2, 3], [1, [[..., ..., ...], 2, 3], 3], [1, 2, [1, [..., ..., ...], 3]], [[1, 2, [..., ..., ...]], 2, 3]]
print(chain)
# output 15592 bytes in use; 401 refs in use
mem()
head = chain[-1]
del chain
# output 15120 bytes in use; 399 refs in use
mem()
del head
# output 72 bytes in use; 3 refs in use
mem()