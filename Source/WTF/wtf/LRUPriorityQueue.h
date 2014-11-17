/*
 * (C) Copyright 2014 Hewlett-Packard Development Company, L.P.
 * This program is free software; you can redistribute it and/or modify it under the
 * terms of version 2.1 of the GNU Lesser General Public License as published by the
 * Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with this program;
 * if not, write to:
 * Free Software Foundation, Inc.
 * 51 Franklin Street, Fifth Floor
 * Boston, MA 02110-1301, USA.
 */

#ifndef _LRU_PRIORITY_QUEUE
#define _LRU_PRIORITY_QUEUE

namespace WTF {

template <class T>
class LRUPriorityQueue {
public:
    LRUPriorityQueue(unsigned capacity) {
        m_data = new T[capacity];
        m_sorted = new T*[capacity];
        m_capacity = capacity;
        m_size = 0;
        m_current = 0;
    }

    ~LRUPriorityQueue() {
        delete [] m_data;
        delete [] m_sorted;
    }

    unsigned size() const {
        return m_size;
    }

    T sorted(unsigned position) const {
        ASSERT(position < m_size);
        if (position >= m_size)
            position = m_size - 1;
        return *m_sorted[position];
    }

    T mostRecent(unsigned position) const {
        ASSERT(position < m_size);
        if (position > m_current)
            return m_data[m_size + m_current - position];
        else
            return m_data[m_current - position];
    }

    void add(T data) {
        unsigned newPosition = findSortedPosition(data);
        if (m_size < m_capacity) {
            // shift everything to the right one spot.
            int elementsToShift = (m_size - newPosition);
            if (elementsToShift > 0)
                memmove_s(m_sorted + newPosition + 1, sizeof(T*) * m_capacity, m_sorted + newPosition, sizeof(T*) * elementsToShift);
            m_size++;
        }
        else {
            unsigned oldDataPos = findSortedPosition(m_data[m_current]);
            ASSERT(*m_sorted[oldDataPos] == m_data[m_current]);
            while (m_sorted[oldDataPos] != &m_data[m_current]) {
                oldDataPos++;
                ASSERT(*m_sorted[oldDataPos] == m_data[m_current]);
            }
            if (newPosition != oldDataPos) {
                if (newPosition < oldDataPos) { // shift right 1.
                    //int elementsToShift = (oldDataPos - newPosition);
                    //memmove_s(m_sorted + newPosition + 1, sizeof(T*) * m_capacity, m_sorted + newPosition, sizeof(T*) * elementsToShift);
                    T** p = m_sorted + oldDataPos;
                    T** end = m_sorted + newPosition;
                    T** next;
                    while (p != end) {
                        next = p - 1;
                        *p = *next;
                        p = next;
                    }
                }
                else { // shift left 1.
                    newPosition--;
                    //int elementsToShift = (newPosition - oldDataPos);
                    //if (elementsToShift > 0)
                    //    memmove_s(m_sorted + oldDataPos, sizeof(T*) * m_capacity, m_sorted + oldDataPos + 1, sizeof(T*) * elementsToShift);
                    T** p = m_sorted + oldDataPos;
                    T** end = m_sorted + newPosition;
                    T** next;
                    while (p != end) {
                        next = p + 1;
                        *p = *next;
                        p = next;
                    }
                }
            }
        }
        m_data[m_current] = data;
        m_sorted[newPosition] = &m_data[m_current];

        m_current++;
        if (m_current == m_capacity)
            m_current = 0;
    }

private:
    unsigned findSortedPosition(T value) {
    /*  for (int i = 0; i < m_size; i++) {
        if (*m_sorted[i] >= value)
            return i;
        }
        return m_size;
    */
        unsigned low = 0;
        unsigned high = m_size;
        unsigned mid;
        T current;
        while (low < high) {
            mid = (low + high) / 2;
            current = *m_sorted[mid];
            if (current < value)
                low = mid + 1;
            else
                high = mid;
        }
        return low;
    }

    // sort pointers to data:

    // m_data[0] = 5
    // m_data[1] = 10
    // m_data[2] = 11
    // m_data[3] = 8
    // m_data[4] = 9

    T * m_data;
    unsigned m_capacity;
    unsigned m_size;
    unsigned m_current;

    T ** m_sorted;

    // m_sorted[0] = &m_data[0] -> 5
    // m_sorted[1] = &m_data[3] -> 8
    // m_sorted[2] = &m_data[4] -> 9
    // m_sorted[3] = &m_data[1] -> 10
    // m_sorted[4] = &m_data[2] -> 11

    // m_current = 4.  that means m_sorted[2] (&m_data[4]) will be removed.
    // how do we know what position m_data[4] is in?
    // either walk the array and compare ( m_capacity <= 10 ? ) or do a binary search.

    // m_sorted[0] = &m_data[0] -> 5
    // m_sorted[1] = &m_data[3] -> 8
    // m_sorted[2] = ** &m_data[4] -> 9 **
    // m_sorted[3] = &m_data[1] -> 10
    // m_sorted[4] = &m_data[2] -> 11

    // add(7) :
    // the number being added could either be in the same position
    // as the item being removed, before that position, or after that
    // position.

    // m_sorted[0] = &m_data[0] -> 5
    // m_sorted[1] = &m_data[4] -> 7 **
    // m_sorted[2] = &m_data[3] -> 8 **
    // m_sorted[3] = &m_data[1] -> 10
    // m_sorted[4] = &m_data[2] -> 11
};

}

#endif
