# mt_csp
mt_csp

Основная версия. Работает на csp-каналах из буста (boost::fibers::buffered_channel).

Архитектура:
Создается N рабочих потоков, у которых есть входные и выходные каналы связи.

Основной поток программы читает файл поблочно. Каждый блок разбивается на части, которые частично перекрывают друг друга (размер перекрытия - pattern length -1).
С каждыой частью хранится её глобальный оффсет в файле.

Части отправляются свободным потокам(каждая своему) и основной поток засыпает во when_any на прием данных из каналов от рабочих потоков. 
При пробуждении он собирает ответы от рабочих потоков, досылает еще необработанные слайсы в освободившиеся потоки и повторяет так до тех пор, пока все части текущего куска данных не будут обработаны.

После обработки всех частей текущего куска, загружается новая часть данных из файла, причем небольшой хвост-перекрытие из предыдущего куска копируется в начало нового.
Новая часть аналогично разбивается на части и передается рабочим потокам.

Так продолжается до тех пор, пока весь файл не будет обработан.

Преимущества схемы:
- поток чтения из файла не ищет символ перевода строки(вместо getlines - блочное чтение) и, тем самым, гораздо быстрее происходит чтение.
- никаких мутексов и гонок за ресурсы, общение с потоками происходит "по выделенным линиям", индивидуальными для каждого потока.
 
Недостатки реализации:
- каналы в бусте я трогал довольно давно, неприятным сюрпризом оказалось отсутствие реализации when_all/when_any мультиплексоров для каналов.
В результате, when_any был эмулирован поллингом. Решение плохое, но альтернатива, предлагаемая бустом в документации - создавать по дополнительному потоку-контроллеру на каждый рабочий поток
и вешать их на mutex/condvar.
- требование вывода количества строк в начале вывода приводит к тому, что статистику по ВСЕМУ файлу прходится копить в памяти, вместо того, что бы обрабатывать результаты по мере
  готовности. В худшем случае это по ~9 байт оверхэда(4 байта на строку и 4 байта на смещение в строке плюс байт найденного символа) на однобайтный паттерн. Если гигабайтный файл проверить паттерном ?, то потребление будет порядка 9 гигабайт. Это можно купировать сжатием результатов парсинга, но я этого не делал.
- поиск конца строки - однобайтовый, '\n'.
- тестов нет.
- соответсвенно, отладкой были найдены только самые явные проблемы, наверняка ворох еще где-ниьудь прячется, ожидая сторонних тестов.
- код причесать не удалось, у нас была договоренность сдать результат в понедельник, что и было сделано.
