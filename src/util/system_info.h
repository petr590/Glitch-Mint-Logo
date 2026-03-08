#ifndef GML_UTIL_GET_SYSTEM_NAME_H
#define GML_UTIL_GET_SYSTEM_NAME_H

/**
 * Читает id и имя дистрибутива и записывает их в переменные *id и *name. Если чтение не удалось, то
 * выводит в консоль сообщение об ошибке и устанавливает *id и *name в NULL. Сами переменные id и
 * name не должны быть NULL. Адреса, возвращённые в эти переменные, указывают на статическую память.
 * При повторном вызове функции get_system_id_and_name данные будут перезаписаны. Рекомендуется
 * вызывать данную функцию только один раз.
 * @param id - адрес для записи id
 * @param name - адрес для записи name
 */
void get_system_id_and_name(const char** id, const char** name);

#endif