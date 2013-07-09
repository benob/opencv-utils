#pragma once

namespace amu {

    // adapted from "Introduction to Algorithms", by Cormen et al.
    template <typename ObjectType, typename ValueType>
        class NBest {
            int num;
            int maxSize;
            ObjectType* objects;
            ValueType* values;

            void swap(const int a, const int b) {
                const ValueType tmpValue = values[a];
                values[a] = values[b];
                values[b] = tmpValue;
                const ObjectType tmpObject = objects[a];
                objects[a] = objects[b];
                objects[b] = tmpObject;
            }

            // ------- implementation for n-max

            void heapifyMin(int index) {
                const int left = index << 1;
                const int right = left + 1;
                int largest = index;
                if(left < num && values[left] < values[index]) largest = left;
                if(right < num && values[right] < values[largest]) largest = right;
                if(largest != index) {
                    swap(index, largest);
                    heapifyMin(largest);
                }
            }

            void buildMin() {
                int index = (num - 1) >> 1;
                while(index > 0) {
                    heapifyMin(index);
                    index >>= 1;
                }
                heapifyMin(index);
            }

            void extractMin() {
                values[0] = values[num - 1];
                objects[0] = objects[num - 1];
                num --;
                heapifyMin(0);
            }

            public:
            void sortNmax() {
                const int savedNum = num;
                int index = num - 1;
                while(index > 0) {
                    swap(0, index);
                    num --;
                    heapifyMin(0);
                    index --;
                }
                num = savedNum;
            }

            void insertNmax(ValueType value, ObjectType object) {
                if(num >= maxSize) {
                    if(value < values[0]) return;
                    extractMin();
                }
                int index = num;
                values[index] = value;
                objects[index] = object;
                num += 1;
                int parent = index >> 1;
                while(index > 0 && values[parent] > values[index]) {
                    swap(index, parent);
                    index = parent;
                    parent >>= 1;
                }
            }

            private:
            // ------- implementation for n-min

            void heapifyMax(int index) {
                const int left = index << 1;
                const int right = left + 1;
                int largest = index;
                if(left < num && values[left] > values[index]) largest = left;
                if(right < num && values[right] > values[largest]) largest = right;
                if(largest != index) {
                    swap(index, largest);
                    heapifyMax(largest);
                }
            }

            void buildMax() {
                int index = (num - 1) >> 1;
                while(index > 0) {
                    heapifyMax(index);
                    index >>= 1;
                }
                heapifyMax(index);
            }

            void extractMax() {
                values[0] = values[num - 1];
                objects[0] = objects[num - 1];
                num --;
                heapifyMax(0);
            }

            public:
            void sortNmin() {
                const int savedNum = num;
                int index = num - 1;
                while(index > 0) {
                    swap(0, index);
                    num --;
                    heapifyMax(0);
                    index --;
                }
                num = savedNum;
            }

            void insertNmin(ValueType value, ObjectType object) {
                if(num >= maxSize) {
                    if(value > values[0]) return;
                    extractMax();
                }
                int index = num;
                values[index] = value;
                objects[index] = object;
                num += 1;
                int parent = index >> 1;
                while(index > 0 && values[parent] < values[index]) {
                    swap(index, parent);
                    index = parent;
                    parent >>= 1;
                }
            }

            /*NBest(const NBest& other) : NBest(other.maxSize) {
                for(int i = 0; i < num; i++) {
                    objects[i] = other.objects[i];
                    values[i] = values.objects[i];
                }
            }*/

            NBest(const int _maxSize) : num(0), maxSize(_maxSize) {
                objects = new ObjectType[maxSize];
                values = new ValueType[maxSize];
            }

            ~NBest() {
                delete objects;
                delete values;
            }

            ObjectType get(const int index) const {
                return objects[index];
            }

            ValueType getValue(const int index) const {
                return values[index];
            }

            int size() const {
                return num;
            }

            void clear() {
                num = 0;
            }
        };

    typedef NBest<int, double> StdNBest;

}
