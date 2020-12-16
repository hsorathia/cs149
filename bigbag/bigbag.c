#include <string.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <netinet/in.h>
#include "bigbag.h"

struct bigbag_entry_s *entry_addr(void *hdr, uint32_t offset) {
    if (offset == 0) return NULL;
    return (struct bigbag_entry_s *)((char*)hdr + offset);
}

uint32_t entry_offset(void *hdr, void *entry) {
    return (uint32_t)((uint64_t)entry - (uint64_t)hdr);
}

void list_all_strings(struct bigbag_hdr_s *hdr, struct bigbag_entry_s *entry);
void contains_element(struct bigbag_hdr_s *hdr, struct bigbag_entry_s *entry, char *element);
void add_element(struct bigbag_hdr_s *hdr, struct bigbag_entry_s *entry, char *element);
void delete_element(struct bigbag_hdr_s *hdr, struct bigbag_entry_s *entry, char *element);

int main(int argc, char **argv) {

    // output when the user inputs incorrect amounts of arguments
    if (argc < 2) {
        printf("USAGE: ./bigbag [-t] filename\n");
        return 1;
    }

    int fd;
    void *file_base;
    // if the user uses the -t flag then they don't want their changes to save into the file
    // if the -t flag is set then we open the argv[2] since that should be the new file name
    // and then we change how we use mmap to MAP_PRIVATE since we don't want to write the changes
    if (strcmp(argv[1], "-t") == 0) {
        fd = open(argv[2], O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
        file_base = mmap(0, BIGBAG_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
        
    } else {
        fd = open(argv[1], O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
        file_base = mmap(0, BIGBAG_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    }
    
    if (file_base == MAP_FAILED) {
        perror("mmap");
        return 3;
    }

    struct bigbag_hdr_s *hdr = file_base;

    struct stat stat;
    fstat(fd, &stat);
    struct bigbag_entry_s *entry;

    if (stat.st_size == 0) {
        // grow the empty file and fill it with 0's
        ftruncate(fd, BIGBAG_SIZE); 
        // now we need to set the header of the entry
        hdr->magic = ntohl(BIGBAG_MAGIC);
        hdr->first_element = 0;
        hdr->first_free = 12; // the header is of size 12
        // our free entry
        struct bigbag_entry_s *free_entry;
        free_entry = entry_addr(hdr, hdr->first_free);
        free_entry->entry_magic = BIGBAG_FREE_ENTRY_MAGIC;
        free_entry->entry_len = BIGBAG_SIZE - sizeof(*hdr);
        free_entry->next = 0;
        // i wan die
    }

    char *strInput = NULL;
    size_t strBuffer = 0;
    ssize_t out;
    while ((out = getline(&strInput, &strBuffer, stdin)) != -1) {
        // output if the user ends up using the wrong commands
        if (strInput[0] != 'l' && strInput[0] != 'c' && strInput[0] != 'a' && strInput[0] != 'd') {
            printf("%c is not used correctly\npossible commands:\na string_to_add\nd string_to_delete\nc string_to_check\nl\n", strInput[0]);
        }

        if (strInput[0] == 'l') {
            list_all_strings(hdr, entry);
        } else {
            // if we're at these commands then we need to split the input into two parts
            // command_char [element]
            // where command_char is l, c, a, or d and element is the string the user wants to
            // check/add/delete
            char *element = strtok(strInput, " ");
            element = strtok(NULL, "");
            if (element != NULL) {
                element[strlen(element) - 1] = 0;
            } else {
                continue;
            }

            if (strInput[0] == 'c') {
                contains_element(hdr, entry, element);
            } else if (strInput[0] == 'a') {
                add_element(hdr, entry, element);
            } else if (strInput[0] == 'd') {
                delete_element(hdr, entry, element);
            }
            
        }
    }

    free(strInput);

}

void list_all_strings(struct bigbag_hdr_s *hdr, struct bigbag_entry_s *entry) {
    /*
        iterate through the linked list and list all the entries
        to do this we:
        1. check to see if the first element in the list exists if the first element is 0, that means that the bag is empty
        2. go through the length of the list and print out the contents of the list

    */
    uint32_t first_elem = hdr->first_element;
    if (first_elem == 0) {
        printf("empty bag\n");
        return;
    }
    entry = entry_addr(hdr, first_elem);
    if (entry->entry_magic == BIGBAG_USED_ENTRY_MAGIC) {
        printf("%s\n", entry->str);
    }
    while (entry->next != 0) {
        // we increment entry at the beginning so we can get all the elements in the list
        entry = entry_addr(hdr, entry->next);
        if (entry->entry_magic == BIGBAG_USED_ENTRY_MAGIC) {
            printf("%s\n", entry->str);
        }
    }
}

void contains_element(struct bigbag_hdr_s *hdr, struct bigbag_entry_s *entry, char *element) {
    /*
        the logic of this is to grab the string the user wants to find and see if it exists in an entry
        in the big bag of strings. if it does, then we set a flag. if the flag is set by the end of the program
        we print "found" else we print "not found"

        the loop iteration is similar to the list method implemented earlier, i used strcmp() method to compare the
        value of the strings
    */
    int found = 0; // 0 -> not found

    uint32_t first_elem = hdr->first_element;

    entry = entry_addr(hdr, first_elem);

    if (entry->entry_magic == BIGBAG_USED_ENTRY_MAGIC) {
        if (strcmp(element, entry->str) == 0) {
            found = 1; // was found
        }
    }
    while (entry->next != 0) {
        entry = entry_addr(hdr, entry->next);
        if (entry->entry_magic == BIGBAG_USED_ENTRY_MAGIC) {
            if (strcmp(element, entry->str) == 0) {
                found = 1; // was found
            }
        }
    }
    if (found == 1) {
        printf("found\n");
    } else {
        printf("not found\n");
    }
}

void add_element(struct bigbag_hdr_s *hdr, struct bigbag_entry_s *entry, char *element) {
    
    /* 
        to add a new element we need to:
        1. set our new entry to point at the current free space
        2. set the new entry to have a used space magic number
        3. set the element size for how much space is being used by the entry
        4. copy the element over into the entry
        
        then we need to:
        6. create a new free entry (so we can split the space properly)
        7. changing the header so it points to the right elements (free space is going to be the offset)
           of the entry we're making right now, and the first element points to the entry offset we created in 1-5
        8. create a new free space entry and set the magic number to the free_entry_magic #
        9. set the length to be the (bag size - length of entry that we just created - size of the header)
        10. set the next offset of the free space to 0
    */

    // create a new entry at the free space
    uint32_t free_space_offset = hdr->first_free;
    uint32_t first_element_offset = hdr->first_element;
    entry = entry_addr(hdr, hdr->first_free);
    entry->next = hdr->first_element; 
    entry->entry_magic = BIGBAG_USED_ENTRY_MAGIC;
    int element_size = strlen(element) + 1; 
    // if the size we're trying to allocate is less than the minimum size, we use the minimum entry size
    if (MIN_ENTRY_SIZE > element_size) {
        element_size = MIN_ENTRY_SIZE;
    } 

    // if the space we're allocating is > than the amount we have free then we have no space left
    if (entry->entry_len < element_size) {
        printf("out of space\n.");
        return;
    } 
    entry->entry_len = element_size;
    strcpy(entry->str, element);
    struct bigbag_entry_s *new_free_entry;
    uint32_t size = entry->entry_len + sizeof(*entry);
    hdr->first_free = hdr->first_free + size;
    
    // we're going to create a new free space thing (to complete the split)
    new_free_entry = entry_addr(hdr, hdr->first_free);
    new_free_entry->entry_magic = BIGBAG_FREE_ENTRY_MAGIC;
    new_free_entry->entry_len = BIGBAG_SIZE - size - sizeof(*hdr);
    new_free_entry->next = 0;
    
    // create two runners so we can add the item into the linked list in a sorted manner
    struct bigbag_entry_s *first, *second;
    first = entry_addr(hdr, first_element_offset);
    second = first;

    /* 
        we have to enter the entry into the proper spot so first
        1. check to see if the entry is smaller (alphabetically) than the first element
           and if it is then we set the new element to point to the beginning of the old list
           and set the header to point to the new elements offset
        2. if the entry is not smaller than the first element then we have to go through
           the list using two pointers and see if we can sqeueze the element in between two others
        3. if we are at the end of the list that means we have to set the old end of the list
           to the entry's offset and the entry to point to 0
    */
    if (first == NULL) {
        entry->next = 0;
        hdr->first_element = free_space_offset; 
    } else if (strcmp(element, first->str) <= 0) {
        entry->next = hdr->first_element;
        hdr->first_element = free_space_offset;
    } else {
        while (first != NULL) {
            if (strcmp(element, first->str) <= 0) {
    
                entry->next = second->next;
                second->next = free_space_offset;   
                break;
            }

            second = first;
            first = entry_addr(hdr, first->next);

            if (first == NULL) {
                entry->next = 0;
                second->next = free_space_offset;
    
                break;
            }
        }
    }
    printf("added %s\n", element);

}

void delete_element(struct bigbag_hdr_s *hdr, struct bigbag_entry_s *entry, char *element) {
    /*
        similar to check, but instead we traverse through the list and then mark the entry 
        with the proper element to free and sets a found flag that we can use to display if the
        element was deleted or not.
    */
    int found = 0;
    uint32_t first_elem = hdr->first_element;
    struct bigbag_entry_s *second;

    entry = entry_addr(hdr, first_elem);
    second = entry;

    if (entry != NULL) {
        // ensure that the second entry is not null 
        if (strcmp(element, entry->str) <= 0) {
            // trying to remove the second element
            entry->entry_magic = BIGBAG_FREE_ENTRY_MAGIC;
            hdr->first_element = entry->next;
            entry->next = 0;
            found = 1;
        } else {
            while (entry != NULL) {
                // iterate through the linked list and see if we find the element we're trying to delete
                // if we find it, then we set the ntry to free and we reset 
                if (strcmp(element, entry->str) <= 0) {
                    entry->entry_magic = BIGBAG_FREE_ENTRY_MAGIC;
                    second->next = entry->next;
                    entry->next = 0;
                    found = 1;
                    break;
                }
                second = entry;
                entry = entry_addr(hdr, entry->next);
            }
        }
    }

    if (found == 1) {
        printf("deleted %s\n", element);
    } else {
        printf("no %s\n", element);
    }

}
