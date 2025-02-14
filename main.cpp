#include <iostream>
#include <pthread.h>
#include <map>
#include <vector>
#include <set>
#include <algorithm>
#include <fstream>
#include <cmath>
#include <numeric>

#define MAX_LENGTH 256
using namespace std;

/**
 * Struct for the files
 */
struct File {
    vector<string> words;
    int id;
    char name[MAX_LENGTH];
    int size;
};

/**
 * Struct for the threads
 */
struct Threads {
    vector<File> *files;
    vector<map<string, set<int>>> *word_map;
    pthread_mutex_t *mutex;
    pthread_barrier_t *barrier;
    int start_letter;
    int end_letter;
    vector<int> *file_indices;
    int *current_index;
};

/**
 * We want to get only the letters from the words and make them lowercase
 */
string lower_letters(const string &input) {
    string result;
    for (char c : input) {
        if (isalpha(c)) {
            result += tolower(c);
        }
    }
    return result;
}

/**
 * The mapping function
 * 
 * Here we will get the words from the files and put them in the word_map,
 * but we will kind of reuse the threads to "steal" the work from another
 * thread if it finishes faster and not make it wait for the others
 * 
 * This way each of the threads are busy bees
 */
void *mapping(void *arg) {
    Threads *args = (Threads *)arg;

    vector<File> *files = args->files;
    vector<int> *file_indices = args->file_indices;

    /**
     * Current index is shared between the threads
     * We will use it to get the file index from the file_indices vector
     */
    int *current_index = args->current_index;

    /**
     * We will use the local map to store the words and the files they are in
     * and then we will merge them with the global map
     */
    map<string, set<int>> local_map;

    while (true) {
        int file_index;
        /**
         * Critical section start
         */
        pthread_mutex_lock(args->mutex);
        /**
         * Check if we got to the end of the files vector
         * If yes, we will break the loop and exit the thread
         * If not, we will get the file index from the file_indices vector
         */
        if (*current_index < static_cast<int>(file_indices->size())) {
            file_index = (*file_indices)[(*current_index)++];
        } else {
            pthread_mutex_unlock(args->mutex);
            break;
        }
        pthread_mutex_unlock(args->mutex);
        /**
         * Critical section end
         */

        /**
         * Get the respective file and add the words (in lowercase) AND the
         * file ids to the local map
         */
        File &file = (*files)[file_index];
        for (string &word : file.words) {
            string lowered_word = lower_letters(word);
            if (!lowered_word.empty()) {
                local_map[lowered_word].insert(file.id);
            }
        }
    }

    /**
     * Critical section start
     */
    pthread_mutex_lock(args->mutex);
    /**
     * Merge the local map with the global map
     */
    (*args->word_map).push_back(local_map);
    pthread_mutex_unlock(args->mutex);
    /**
     * Critical section end
     */

    /**
     * Now, because we need to run the mappers before the reducers and
     * BECAUSE THE HOMEWORK MADE US CREATE THE THREADS IN A SINGLE FOR LOOP
     * we will need a barrier to make the reducers wait for the mappers
     */
    pthread_barrier_wait(args->barrier);
    pthread_exit(NULL);
}

/**
 * The reducing function
 * 
 * Here we will get the words from the word_map and sort them by the number
 * of files they are in and alphabetically and put them in the respective
 * files
 */
void *reducing(void *arg) {
    Threads *args = (Threads *)arg;
    /**
     * Wait for the mappers to finish
     */
    pthread_barrier_wait(args->barrier);
    map<string, vector<int>> words;

    /**
     * Get the words from the word_map and put them in the local_word_map vector
     * 
     * This way we will not search in the entire word_map for the words
     * that start with the respective letter and (maybe) reduce the complexity
     * (reduce...get it? :D)
     */
    for (auto& part : *args->word_map) {
        for (auto& word_part : part) {
            char first_letter = word_part.first[0];
            if (first_letter >= 'a' + args->start_letter && first_letter <= 'a' + args->end_letter) {
                /**
                 * In a way sort the file ids and remove the duplicates
                 */
                set<int> unique_files(words[word_part.first].begin(), words[word_part.first].end());
                unique_files.insert(word_part.second.begin(), word_part.second.end());
                words[word_part.first] = vector<int>(unique_files.begin(), unique_files.end());
            }
        }
    }

    vector<pair<string, vector<int>>> local_word_map(words.begin(), words.end());

    /**
     * Sort the local_word_map vector by the number of files they are in and alphabetically
     */
    sort(local_word_map.begin(), local_word_map.end(), [](const pair<string, vector<int>> &a,
                                        const pair<string, vector<int>> &b) {
        if (a.second.size() != b.second.size()) {
            return a.second.size() > b.second.size();
        }
        return a.first < b.first;
    });

    /**
     * Write the words in the respective files
     */
    for (char letter = 'a' + args->start_letter; letter <= 'a' + args->end_letter; letter++) {
        string filename = string(1, letter) + ".txt";
        ofstream outfile(filename);

        /**
         * Get the words that start with the respective letter which
         * need to be written in the respective file
         * 
         * ...respectively
         * 
         * Also C++ TIME
         */
        for (auto &word_entry : local_word_map) {
            if (word_entry.first[0] == letter) {
                outfile << word_entry.first << ":[";
                for (size_t i = 0; i < word_entry.second.size(); i++) {
                    outfile << word_entry.second[i];
                    /**
                     * We need to put space between the file ids and a "]"
                     * after the last file id
                     */
                    if (i != word_entry.second.size() - 1) {
                        outfile << " ";
                    }
                }
                /**
                 * We need to put a "]" after the last file id
                 */
                outfile << "]" << endl;
            }
        }
        outfile.close();
    }

    pthread_exit(NULL);
}

int main(int argc, char **argv) {
    /**
     * If you miss this then you're gonna have a bad time
     */
    if (argc < 4) {
        cout << "Usage: ./tema1 <numar_mapperi> <numar_reduceri> <fisier_intrare>" << endl;
        return 1;
    }

    /**
     * Get the inputs
     */
    int number_of_mappers = atoi(argv[1]);
    int number_of_reducers = atoi(argv[2]);
    string input_file = argv[3];

    /**
     * Get the file
     */
    FILE *file = fopen(input_file.c_str(), "r");
    if (!file) {
        perror("File not found");
        return 1;
    }

    /**
     * Read number of files
     */
    int number_of_files;
    if (!fscanf(file, "%d", &number_of_files)) {
        perror("Error reading number of files");
        return 1;
    }

    vector<File> files(number_of_files);

    /**
     * Set the file vector with ids, names and words
     */
    for (int i = 0; i < number_of_files; i++) {
        files[i].id = i + 1;
        if (!fscanf(file, "%s", files[i].name)) {
            perror("Error reading file name");
            return 1;
        }
        /**
         * Time to spice up some things
         * C++ TIME
         * The name of the file is also the path it is on so...don't judge me
         */
        string path = string(files[i].name);

        ifstream text_file(path);
        if (!text_file.is_open()) {
            perror("File not found");
            return 1;
        }

        string word;
        /**
         * Get the words of the file + the size because we need it
         * for sorting the files later
         */
        while (text_file >> word) {
            files[i].words.push_back(word);
            files[i].size += word.length();
        }

        text_file.close();
    }

    fclose(file);

    pthread_t threads[number_of_mappers + number_of_reducers];
    vector<map<string, set<int>>> word_map;
    pthread_mutex_t mutex;
    pthread_barrier_t barrier;

    /**
     * Inits for mutex and barrier
     */
    pthread_mutex_init(&mutex, NULL);
    pthread_barrier_init(&barrier, NULL, number_of_mappers + number_of_reducers);

    /**
     * Sort files by size because we want to start with the
     * biggest files first like in the example in the
     * homework's document with 5MB, 5MB and 1MB files
     */
    vector<int> file_indices(number_of_files);
    iota(file_indices.begin(), file_indices.end(), 0);
    sort(file_indices.begin(), file_indices.end(), [&](int a, int b) {
        return files[a].size > files[b].size;
    });

    int current_index = 0;

    /**
     * Create threads for mappers and reducers
     */
    vector<Threads> thread_args(number_of_mappers + number_of_reducers);
    int letters_per_reducer = ceil(26.0 / number_of_reducers);

    /**
     * IN A SINGLE FOR LOOP
     * I created threads for mappers and reducers
     * This way i achieved M + R threads in a single for loop
     */
    for (int i = 0; i < number_of_mappers + number_of_reducers; i++) {
        if (i < number_of_mappers) {
            thread_args[i].files = &files;
            thread_args[i].word_map = &word_map;
            thread_args[i].mutex = &mutex;
            thread_args[i].barrier = &barrier;
            thread_args[i].file_indices = &file_indices;
            thread_args[i].current_index = &current_index;
            pthread_create(&threads[i], NULL, mapping, &thread_args[i]);
        } else {
            int reducer_index = i - number_of_mappers;
            thread_args[i].word_map = &word_map;
            thread_args[i].mutex = &mutex;
            thread_args[i].barrier = &barrier;
            thread_args[i].start_letter = reducer_index * letters_per_reducer;
            thread_args[i].end_letter = min(25, (reducer_index + 1) * letters_per_reducer - 1);
            pthread_create(&threads[i], NULL, reducing, &thread_args[i]);
        }
    }

    /**
     * Join the threads
     */
    for (int i = 0; i < number_of_mappers + number_of_reducers; i++) {
        pthread_join(threads[i], NULL);
    }

    /**
     * "We're destroyers" -- God of War Ragnarok
     */
    pthread_mutex_destroy(&mutex);
    pthread_barrier_destroy(&barrier);

    return 0;
}
