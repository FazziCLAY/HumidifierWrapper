#ifndef DATA_ARRAY_H
#define DATA_ARRAY_H

class DataArray
{
public:
    DataArray(int size)
    {
        _size = size;
        _arr = new int[_size]; // Динамическое выделение памяти для массива
        _median = 0;
    }

    ~DataArray() {
        delete[] _arr; // Очистка выделенной памяти в деструкторе
    }

    void put(int num) {
        for (int i = _size - 1; i > 0; i--) {
            _arr[i] = _arr[i - 1];
        }
        _arr[0] = num;
        recalcMedian(); // Пересчитываем медиану каждый раз, когда добавляем новый элемент
    }

    int get() {
        return _median;
    }

private:
    int _size;
    int _median;
    int* _arr; // Указатель на массив

    void recalcMedian() {
        _median = 0;
        for (int i = 0; i < _size; i++) {
            _median += _arr[i];
        }
        _median /= _size;
    }
};

#endif
