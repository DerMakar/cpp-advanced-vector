# Аналог vector с размещением в сырой памяти
Готовый класс Vector с коллекцией методов работы с контейром, который можно включить в другой проект. Работа конструктора контейнера построена на выделении сырой памяти. Вариативные шаблоны позволяют использовать контейнер для любых конструируемых типов. Методы класса минимизируют копирование значений - в основном идет перемещение значений или непосредстевнная из инициализация внутри контейнера. За счет этого класс эффективен с т.з. памяти и скорости работы.

# Использование
1. Файлы проекта можно включить в другой проект и создавать объекты типа Vector

# Системные требования
1. C++17
2. GCC (MinGW-w64) 11.2.0

# Примечание
Тренировочный проект в рамках ЯндексПрактикума с пошаговым созданием с нуля контейнера, аналогичного vector, для отработки навыков с работы с сырой памятью.
