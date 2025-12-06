#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>

// Структура для хранения флагов
typedef struct {
    int recursive;  // -R
    int long_format;// -l
    int show_group; // -g
} Flags;

// Функция для разбора аргументов командной строки
void parse_args(int argc, char *argv[], Flags *flags, char ***targets, int *target_count) {
    flags->recursive = 0;
    flags->long_format = 0;
    flags->show_group = 0;
    *target_count = 0;
    
    // Выделяем память под массив целей
    *targets = malloc(argc * sizeof(char*));
    
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            // Обрабатываем флаги
            for (int j = 1; argv[i][j] != '\0'; j++) {
                switch (argv[i][j]) {
                    case 'R':
                        flags->recursive = 1;
                        break;
                    case 'l':
                        flags->long_format = 1;
                        break;
                    case 'g':
                        flags->show_group = 1;
			flags->long_format = 1;
                        break;
                    default:
                        fprintf(stderr, "Неизвестный флаг: -%c\n", argv[i][j]);
                        break;
                }
            }
        } else {
            // Добавляем цель в список
            (*targets)[(*target_count)++] = argv[i];
        }
    }
    
    // Если целей нет, добавляем текущий каталог
    if (*target_count == 0) {
        (*targets)[0] = ".";
        *target_count = 1;
    }
}

// Функция для получения строкового представления типа файла и прав доступа
void get_mode_string(mode_t mode, char *str) {
    // Тип файла
    if (S_ISDIR(mode)) str[0] = 'd';
    else if (S_ISCHR(mode)) str[0] = 'c';
    else if (S_ISBLK(mode)) str[0] = 'b';
    else if (S_ISFIFO(mode)) str[0] = 'p';
    else if (S_ISLNK(mode)) str[0] = 'l';
    else if (S_ISSOCK(mode)) str[0] = 's';
    else str[0] = '-';
    
    // Права доступа пользователя
    str[1] = (mode & S_IRUSR) ? 'r' : '-';
    str[2] = (mode & S_IWUSR) ? 'w' : '-';
    str[3] = (mode & S_IXUSR) ? 'x' : '-';
    
    // Права доступа группы
    str[4] = (mode & S_IRGRP) ? 'r' : '-';
    str[5] = (mode & S_IWGRP) ? 'w' : '-';
    str[6] = (mode & S_IXGRP) ? 'x' : '-';
    
    // Права доступа других
    str[7] = (mode & S_IROTH) ? 'r' : '-';
    str[8] = (mode & S_IWOTH) ? 'w' : '-';
    str[9] = (mode & S_IXOTH) ? 'x' : '-';
    
    str[10] = '\0';
}

// Функция для обработки символической ссылки
void process_symlink(const char *path, const char *name, Flags flags) {
    char link_target[1024];
    ssize_t len = readlink(path, link_target, sizeof(link_target) - 1);
    
    if (len != -1) {
        link_target[len] = '\0';
        
        // Проверяем, является ли цель ссылки также символической ссылкой
        struct stat target_stat;
        if (lstat(link_target, &target_stat) == 0 && S_ISLNK(target_stat.st_mode)) {
            char second_target[1024];
            ssize_t second_len = readlink(link_target, second_target, sizeof(second_target) - 1);
            
            if (second_len != -1) {
                second_target[second_len] = '\0';
                printf("%s -> %s -> %s\n", name, link_target, second_target);
            } else {
                printf("%s -> %s\n", name, link_target);
            }
        } else {
            printf("%s -> %s\n", name, link_target);
        }
    } else {
        printf("%s\n", name);
    }
}

// Функция для вывода информации о файле в длинном формате
void print_long_format(const char *path, const char *name, struct stat *file_stat, Flags flags) {
    char mode_str[11];
    get_mode_string(file_stat->st_mode, mode_str);
    
    // Получаем информацию о владельце и группе
    struct passwd *pw = getpwuid(file_stat->st_uid);
    struct group *gr = getgrgid(file_stat->st_gid);
    
    // Форматируем время модификации
    char time_str[20];
    struct tm *timeinfo = localtime(&file_stat->st_mtime);
    strftime(time_str, sizeof(time_str), "%b %d %H:%M", timeinfo);
    
    // Выводим информацию
    printf("%s %2ld", mode_str, (long)file_stat->st_nlink);
    
    if (!flags.show_group) {
        printf(" %-8s", pw ? pw->pw_name : "?");
    }
    
    if (flags.show_group) {
        printf(" %-8s", gr ? gr->gr_name : "?");
    }
    
    printf(" %8lld", (long long)file_stat->st_size);
    printf(" %s", time_str);
    printf(" %s", name);
    
    // Если это символическая ссылка, выводим цель
    if (S_ISLNK(file_stat->st_mode)) {
        char link_target[1024];
        ssize_t len = readlink(path, link_target, sizeof(link_target) - 1);
        if (len != -1) {
            link_target[len] = '\0';
            printf(" -> %s", link_target);
        }
    }
    
    printf("\n");
}

// Функция для обработки одного файла/каталога
void process_entry(const char *path, const char *name, Flags flags, int is_dir) {
    struct stat file_stat;
    char full_path[1024];
    
    // Формируем полный путь
    if (strcmp(path, ".") == 0) {
        snprintf(full_path, sizeof(full_path), "%s", name);
    } else {
        snprintf(full_path, sizeof(full_path), "%s/%s", path, name);
    }
    
    // Получаем информацию о файле
    if (lstat(full_path, &file_stat) == -1) {
        perror("lstat");
        return;
    }
    
    // Выводим информацию в зависимости от флагов
    if (flags.long_format) {
        print_long_format(full_path, name, &file_stat, flags);
    } else {
        if (S_ISLNK(file_stat.st_mode)) {
            process_symlink(full_path, name, flags);
        } else {
            printf("%s\n", name);
        }
    }
}

// Функция для обработки каталога
void process_directory(const char *path, Flags flags, int recursive_call) {
    DIR *dir;
    struct dirent *entry;
    
    if ((dir = opendir(path)) == NULL) {
        perror("opendir");
        return;
    }
    
    // Выводим путь к каталогу при рекурсивном обходе
    if (recursive_call) {
        printf("\n%s:\n", path);
    }

    // Читаем содержимое каталога
    while ((entry = readdir(dir)) != NULL) {
        // Пропускаем скрытые файлы (те, что начинаются с точки)
        if (entry->d_name[0] == '.' && !flags.long_format) {
            continue;
        }
        
        // Пропускаем . и ..
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        process_entry(path, entry->d_name, flags, 0);
    }
    
    closedir(dir);
    
    // Рекурсивный обход подкаталогов
    if (flags.recursive) {
        dir = opendir(path);
        
        while ((entry = readdir(dir)) != NULL) {
            // Пропускаем . и ..
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                continue;
            }
            
            char full_path[1024];
            snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);
            
            struct stat file_stat;
            if (lstat(full_path, &file_stat) == 0 && S_ISDIR(file_stat.st_mode)) {
                process_directory(full_path, flags, 1);
            }
        }
        
        closedir(dir);
    }
}

// Основная функция
int main(int argc, char *argv[]) {
    Flags flags;
    char **targets;
    int target_count;
    
    // Парсим аргументы
    parse_args(argc, argv, &flags, &targets, &target_count);
    
    // Обрабатываем каждую цель
    for (int i = 0; i < target_count; i++) {
        struct stat path_stat;
        
        if (stat(targets[i], &path_stat) == -1) {
            perror("stat");
            continue;
        }
        
        if (S_ISDIR(path_stat.st_mode)) {
            // Если цель - каталог
            if (target_count > 1) {
                printf("%s:\n", targets[i]);
            }
            process_directory(targets[i], flags, 0);
            
            if (i < target_count - 1) {
                printf("\n");
            }
        } else {
            // Если цель - файл
            process_entry(".", targets[i], flags, 0);
        }
    }
    
    // Освобождаем память
    free(targets);
    
    return 0;
}
