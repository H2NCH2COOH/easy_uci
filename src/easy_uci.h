#ifndef _EASY_UCI_H_
#define _EASY_UCI_H_

#include <stddef.h>
#include <stdbool.h>

typedef struct
{
    const char** list;
    size_t len;
} easy_uci_list;

/**
 * easy_uci_register_error_logger: register a function that will be called when error occurred
 * @param logger: the pointer to a logger function
 */
void easy_uci_register_error_logger(void(*logger)(const char*));

/**
 * easy_uci_free_list: free an easy_uci_list filled by some easy_uci functions
 * @param list_p: the pointer to the easy_uci_list to free
 * @return: no return
 */
void easy_uci_free_list(easy_uci_list* list_p);

/**
 * easy_uci_get_section_type: get the type of a section
 * @param package: the name of the package
 * @param section: the name of the section
 * @param buff: the type of the section will be stored in it with '\0' term
 * @param size: the maximum size of buff
 * @return: 0 for success, -1 for failure
 */
int easy_uci_get_section_type(const char* package,const char* section,char* buff,size_t size);

/**
 * easy_uci_add_section: add a new section to package
 * @param package: the name of the package
 * @param type: the type of the new section
 * @param name: the name of the new section, NULL or "" for anonymous section
 * @return: 0 for success, -1 for failure
 *
 * If a section with the same type and name already exists, this function will succeed
 * If a section with the same name and a different type already exists, this function will fail
 */
int easy_uci_add_section(const char* package,const char* type,const char* name);

/**
 * easy_uci_delete_section: delete a section from package
 * @param package: the name of the package
 * @param section: the name of the section
 * @return: 0 for success, -1 for failure
 *
 * When deleting a section that doesn't exist, this function will succeed
 */
int easy_uci_delete_section(const char* package,const char* section);

/**
 * easy_uci_get_all_section_of_type: get all sections of type from package
 * @param package: the name of the package
 * @param type: the type of the sections
 * @param list_p: the pointer to a easy_uci_list for output
 * @return: 0 for success, -1 for failure
 *
 * The list_p->list will be an array of strings each being the name of a section
 * The list_p->len will be the length of the name array
 * If there is no section of type, the list_p->len will be set to 0 list_p->list set to NULL
 * If this function fails, the content of *list_p will not be changed
 * If this function succeeds, *list_p must be freed by easy_uci_free_list()
 * This function will BREAK when there're more then 1024 sections of the type
 *
 * For anonymous sections, this function will return a internal name that can be used by other easy_uci functions
 */
int easy_uci_get_all_section_of_type(const char* package,const char* type,easy_uci_list* list_p);

/**
 * easy_uci_get_nth_section_of_type: get the nth section of type from package
 * @param package: the name of the package
 * @param type: the type of the sections
 * @param n: the index of the section
 * @param name_p: the pointer to a char* that will point to the name of the section
 * @return: 0 for success, -1 for failure
 *
 * The index starts at 0 and can be negative, -1 means the last one and -2 means the second last one and so on
 * The char* pointed by name_p will be set to the name of the section
 * If this function fails, *name_p will not be changed
 * If this function succeeds, *name_p must be freed by using free()
 *
 * For anonymous sections, this function will return a internal name that can be used by other easy_uci functions
 */
int easy_uci_get_nth_section_of_type(const char* package,const char* type,int n,char** name_p);

/**
 * easy_uci_get_option_string: get the value of an option of type string
 * @param package: the name of the package
 * @param section: the name of the section
 * @param option: the name of the option
 * @param buff: the value of the option will be stored in it with '\0' term
 * @param size: the maximum size of the buff
 * @return: 0 for success, -1 for failure
 *
 * If the opption can't be found, this function fails and the content of buff will not be changed
 */
int easy_uci_get_option_string(const char* package,const char* section,const char* option,char* buff,size_t size);

/**
 * easy_uci_set_option_string: set the value of an option of type string
 * @param package: the name of the package
 * @param section: the name of the section
 * @param option: the name of the option
 * @param value: the value for the option
 * @return: 0 for success, -1 for failure
 *
 * If either the package or the section can't be found, this function will fail
 * If the option exists, its value will be replaced and its type changed to string
 * If the value is NULL or "", this function will fail
 */
int easy_uci_set_option_string(const char* package,const char* section,const char* option,const char* value);

/**
 * easy_uci_get_option_list: get the list of an option of type list
 * @param package: the name of the package
 * @param section: the name of the section
 * @param option: the name of the option
 * @param list_p: the pointer to a easy_uci_list for output
 * @return: 0 for success, -1 for failure
 *
 * If the opption can't be found, this function fails and the content of *list_p will not be changed
 */
int easy_uci_get_option_list(const char* package,const char* section,const char* option,easy_uci_list* list_p);

/**
 * easy_uci_set_option_list: set the value of an option of type list
 * @param package: the name of the package
 * @param section: the name of the section
 * @param option: the name of the option
 * @param list_p: the pointer to a easy_uci_list
 * @return: 0 for success, -1 for failure
 *
 * If either the package or the section can't be found, this function will fail
 * If the option exists, its value will be replaced and its type changed to list
 * If the *list_p is empty, this function will fail
 * This function will not modify the content of *list_p
 */
int easy_uci_set_option_list(const char* package,const char* section,const char* option,easy_uci_list* list_p);

/**
 * easy_uci_append_to_option_list: append value to an option of type list
 * @param package: the name of the package
 * @param section: the name of the section
 * @param option: the name of the option
 * @param value: the value to append
 * @return: 0 for success, -1 for failure
 *
 * If either the package or the section can't be found, this function will fail
 * If the option exists and is of type string, it will be converted to a list and then append the value
 * If the value is NULL, this function will fail
 */
int easy_uci_append_to_option_list(const char* package,const char* section,const char* option,const char* value);

/**
 * easy_uci_delete_option: delete an option from section
 * @param package: the name of the package
 * @param section: the name of the section
 * @param option: the name of the option
 *
 * If the option doesn't exist, this function will succeed
 */
int easy_uci_delete_option(const char* package,const char* section,const char* option);

#endif /* _EASY_UCI_H_ */
