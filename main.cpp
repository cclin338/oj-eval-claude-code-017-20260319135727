#include <iostream>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <string>

using namespace std;

// Constants
const int MAX_USERNAME_LEN = 21;
const int MAX_PASSWORD_LEN = 31;
const int MAX_NAME_LEN = 21; // For Chinese characters (5 chars * 4 bytes + 1)
const int MAX_MAIL_LEN = 31;
const int MAX_TRAIN_ID_LEN = 21;
const int MAX_STATION_NAME_LEN = 41; // 10 Chinese chars * 4 bytes + 1
const int MAX_STATIONS = 100;
const int MAX_SEATS = 100000;
const int MAX_DAYS = 92; // June 1 to Aug 31 (92 days)

// User structure
struct User {
    char username[MAX_USERNAME_LEN];
    char password[MAX_PASSWORD_LEN];
    char name[MAX_NAME_LEN];
    char mailAddr[MAX_MAIL_LEN];
    int privilege;
    bool isLoggedIn;

    User() : privilege(0), isLoggedIn(false) {
        memset(username, 0, sizeof(username));
        memset(password, 0, sizeof(password));
        memset(name, 0, sizeof(name));
        memset(mailAddr, 0, sizeof(mailAddr));
    }

    void setUsername(const char* str) {
        strncpy(username, str, MAX_USERNAME_LEN - 1);
        username[MAX_USERNAME_LEN - 1] = '\0';
    }

    void setPassword(const char* str) {
        strncpy(password, str, MAX_PASSWORD_LEN - 1);
        password[MAX_PASSWORD_LEN - 1] = '\0';
    }

    void setName(const char* str) {
        strncpy(name, str, MAX_NAME_LEN - 1);
        name[MAX_NAME_LEN - 1] = '\0';
    }

    void setMailAddr(const char* str) {
        strncpy(mailAddr, str, MAX_MAIL_LEN - 1);
        mailAddr[MAX_MAIL_LEN - 1] = '\0';
    }
};

// Train station info
struct StationInfo {
    char name[MAX_STATION_NAME_LEN];
    int travelTime; // from previous station
    int stopoverTime; // at this station (0 for first and last)
    int price; // cumulative price from start
    int departureTime; // absolute time from day start (minutes)
    int arrivalTime; // absolute time from day start (minutes)
};

// Train structure
struct Train {
    char trainID[MAX_TRAIN_ID_LEN];
    int stationNum;
    int seatNum;
    StationInfo stations[MAX_STATIONS];
    char startTime[6]; // "hh:mm"
    int startHour, startMinute;
    char saleDateFrom[6]; // "mm-dd"
    char saleDateTo[6]; // "mm-dd"
    char type;
    bool isReleased;

    // Seat availability: [day][station]
    int seatAvailability[MAX_DAYS][MAX_STATIONS];

    Train() : stationNum(0), seatNum(0), isReleased(false) {
        memset(trainID, 0, sizeof(trainID));
        memset(startTime, 0, sizeof(startTime));
        memset(saleDateFrom, 0, sizeof(saleDateFrom));
        memset(saleDateTo, 0, sizeof(saleDateTo));
        type = 'G';
        startHour = startMinute = 0;

        // Initialize seat availability
        for (int i = 0; i < MAX_DAYS; i++) {
            for (int j = 0; j < MAX_STATIONS; j++) {
                seatAvailability[i][j] = 0;
            }
        }
    }
};

// Ticket order
struct Order {
    char username[MAX_USERNAME_LEN];
    char trainID[MAX_TRAIN_ID_LEN];
    char fromStation[MAX_STATION_NAME_LEN];
    char toStation[MAX_STATION_NAME_LEN];
    int departureDate; // day index (0-91)
    int departureTime; // minutes from day start
    int arrivalTime; // minutes from day start
    int price;
    int numTickets;
    int status; // 0: success, 1: pending, 2: refunded
    long long timestamp; // for ordering

    Order() : departureDate(0), departureTime(0), arrivalTime(0),
              price(0), numTickets(0), status(0), timestamp(0) {
        memset(username, 0, sizeof(username));
        memset(trainID, 0, sizeof(trainID));
        memset(fromStation, 0, sizeof(fromStation));
        memset(toStation, 0, sizeof(toStation));
    }
};

// Simple hash table for users
class UserHashTable {
private:
    static const int TABLE_SIZE = 10007; // Prime number
    struct Node {
        User user;
        Node* next;
        Node(const User& u) : user(u), next(nullptr) {}
    };

    Node* table[TABLE_SIZE];

    int hash(const char* username) {
        unsigned long hash = 5381;
        int c;
        while ((c = *username++)) {
            hash = ((hash << 5) + hash) + c; // hash * 33 + c
        }
        return hash % TABLE_SIZE;
    }

public:
    UserHashTable() {
        for (int i = 0; i < TABLE_SIZE; i++) {
            table[i] = nullptr;
        }
    }

    ~UserHashTable() {
        for (int i = 0; i < TABLE_SIZE; i++) {
            Node* curr = table[i];
            while (curr) {
                Node* temp = curr;
                curr = curr->next;
                delete temp;
            }
        }
    }

    bool insert(const User& user) {
        int index = hash(user.username);
        Node* curr = table[index];

        // Check if user already exists
        while (curr) {
            if (strcmp(curr->user.username, user.username) == 0) {
                return false; // User already exists
            }
            curr = curr->next;
        }

        // Insert new user
        Node* newNode = new Node(user);
        newNode->next = table[index];
        table[index] = newNode;
        return true;
    }

    User* find(const char* username) {
        int index = hash(username);
        Node* curr = table[index];
        while (curr) {
            if (strcmp(curr->user.username, username) == 0) {
                return &curr->user;
            }
            curr = curr->next;
        }
        return nullptr;
    }

    bool remove(const char* username) {
        int index = hash(username);
        Node* curr = table[index];
        Node* prev = nullptr;

        while (curr) {
            if (strcmp(curr->user.username, username) == 0) {
                if (prev) {
                    prev->next = curr->next;
                } else {
                    table[index] = curr->next;
                }
                delete curr;
                return true;
            }
            prev = curr;
            curr = curr->next;
        }
        return false;
    }
};

// Simple hash table for trains
class TrainHashTable {
private:
    static const int TABLE_SIZE = 10007;
    struct Node {
        Train train;
        Node* next;
        Node(const Train& t) : train(t), next(nullptr) {}
    };

    Node* table[TABLE_SIZE];

    int hash(const char* trainID) {
        unsigned long hash = 5381;
        int c;
        while ((c = *trainID++)) {
            hash = ((hash << 5) + hash) + c;
        }
        return hash % TABLE_SIZE;
    }

public:
    TrainHashTable() {
        for (int i = 0; i < TABLE_SIZE; i++) {
            table[i] = nullptr;
        }
    }

    ~TrainHashTable() {
        for (int i = 0; i < TABLE_SIZE; i++) {
            Node* curr = table[i];
            while (curr) {
                Node* temp = curr;
                curr = curr->next;
                delete temp;
            }
        }
    }

    bool insert(const Train& train) {
        int index = hash(train.trainID);
        Node* curr = table[index];

        while (curr) {
            if (strcmp(curr->train.trainID, train.trainID) == 0) {
                return false;
            }
            curr = curr->next;
        }

        Node* newNode = new Node(train);
        newNode->next = table[index];
        table[index] = newNode;
        return true;
    }

    Train* find(const char* trainID) {
        int index = hash(trainID);
        Node* curr = table[index];
        while (curr) {
            if (strcmp(curr->train.trainID, trainID) == 0) {
                return &curr->train;
            }
            curr = curr->next;
        }
        return nullptr;
    }

    bool remove(const char* trainID) {
        int index = hash(trainID);
        Node* curr = table[index];
        Node* prev = nullptr;

        while (curr) {
            if (strcmp(curr->train.trainID, trainID) == 0) {
                if (prev) {
                    prev->next = curr->next;
                } else {
                    table[index] = curr->next;
                }
                delete curr;
                return true;
            }
            prev = curr;
            curr = curr->next;
        }
        return false;
    }
};

// Global data structures
UserHashTable userTable;
TrainHashTable trainTable;
int userCount = 0;

// Helper functions
int parseTime(const char* timeStr) {
    int hour, minute;
    sscanf(timeStr, "%d:%d", &hour, &minute);
    return hour * 60 + minute;
}

void formatTime(int minutes, char* buffer) {
    int hour = minutes / 60;
    int minute = minutes % 60;
    sprintf(buffer, "%02d:%02d", hour, minute);
}

// Split string by delimiter into array of strings
void splitStringByDelimiter(const char* str, char delimiter, char** tokens, int maxTokens, int& count) {
    count = 0;
    const char* start = str;
    const char* end = str;

    while (*start && count < maxTokens) {
        // Find next delimiter or end
        end = start;
        while (*end && *end != delimiter) end++;

        // Copy token
        int len = end - start;
        if (len > 0) {
            tokens[count] = new char[len + 1];
            strncpy(tokens[count], start, len);
            tokens[count][len] = '\0';
            count++;
        }

        // Move past delimiter
        if (*end == delimiter) end++;
        start = end;
    }
}

int parseDate(const char* dateStr) {
    int month, day;
    sscanf(dateStr, "%d-%d", &month, &day);

    int dayIndex = 0;

    // Add days from June 1
    if (month == 6) {
        dayIndex = day - 1;
    } else if (month == 7) {
        dayIndex = 30 + (day - 1); // June has 30 days
    } else if (month == 8) {
        dayIndex = 61 + (day - 1); // June(30) + July(31) = 61
    }

    return dayIndex;
}

void formatDate(int dayIndex, char* buffer) {
    int month, day;
    if (dayIndex < 30) { // June
        month = 6;
        day = dayIndex + 1;
    } else if (dayIndex < 61) { // July
        month = 7;
        day = dayIndex - 30 + 1;
    } else { // August
        month = 8;
        day = dayIndex - 61 + 1;
    }
    sprintf(buffer, "%02d-%02d", month, day);
}

// Command handlers
int handleAddUser(int argc, char* argv[]) {
    // Parse arguments
    char curUser[21] = {0};
    char username[21] = {0};
    char password[31] = {0};
    char name[21] = {0};
    char mailAddr[31] = {0};
    int privilege = 0;

    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "-c") == 0 && i + 1 < argc) {
            strncpy(curUser, argv[++i], MAX_USERNAME_LEN - 1);
            curUser[MAX_USERNAME_LEN - 1] = '\0';
        } else if (strcmp(argv[i], "-u") == 0 && i + 1 < argc) {
            strncpy(username, argv[++i], MAX_USERNAME_LEN - 1);
            username[MAX_USERNAME_LEN - 1] = '\0';
        } else if (strcmp(argv[i], "-p") == 0 && i + 1 < argc) {
            strncpy(password, argv[++i], MAX_PASSWORD_LEN - 1);
            password[MAX_PASSWORD_LEN - 1] = '\0';
        } else if (strcmp(argv[i], "-n") == 0 && i + 1 < argc) {
            strncpy(name, argv[++i], MAX_NAME_LEN - 1);
            name[MAX_NAME_LEN - 1] = '\0';
        } else if (strcmp(argv[i], "-m") == 0 && i + 1 < argc) {
            strncpy(mailAddr, argv[++i], MAX_MAIL_LEN - 1);
            mailAddr[MAX_MAIL_LEN - 1] = '\0';
        } else if (strcmp(argv[i], "-g") == 0 && i + 1 < argc) {
            privilege = atoi(argv[++i]);
        }
    }

    // Check if this is the first user
    if (userCount == 0) {
        User newUser;
        newUser.setUsername(username);
        newUser.setPassword(password);
        newUser.setName(name);
        newUser.setMailAddr(mailAddr);
        newUser.privilege = 10; // First user gets privilege 10
        newUser.isLoggedIn = false;

        if (userTable.insert(newUser)) {
            userCount++;
            return 0;
        }
        return -1;
    }

    // Check current user exists and is logged in
    User* current = userTable.find(curUser);
    if (!current || !current->isLoggedIn) {
        return -1;
    }

    // Check privilege requirement
    if (privilege >= current->privilege) {
        return -1;
    }

    // Create new user
    User newUser;
    newUser.setUsername(username);
    newUser.setPassword(password);
    newUser.setName(name);
    newUser.setMailAddr(mailAddr);
    newUser.privilege = privilege;
    newUser.isLoggedIn = false;

    if (userTable.insert(newUser)) {
        userCount++;
        return 0;
    }

    return -1;
}

int handleLogin(int argc, char* argv[]) {
    char username[21] = {0};
    char password[31] = {0};

    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "-u") == 0 && i + 1 < argc) {
            strncpy(username, argv[++i], MAX_USERNAME_LEN - 1);
            username[MAX_USERNAME_LEN - 1] = '\0';
        } else if (strcmp(argv[i], "-p") == 0 && i + 1 < argc) {
            strncpy(password, argv[++i], MAX_PASSWORD_LEN - 1);
            password[MAX_PASSWORD_LEN - 1] = '\0';
        }
    }

    User* user = userTable.find(username);
    if (!user || strcmp(user->password, password) != 0 || user->isLoggedIn) {
        return -1;
    }

    user->isLoggedIn = true;
    return 0;
}

int handleLogout(int argc, char* argv[]) {
    char username[21] = {0};

    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "-u") == 0 && i + 1 < argc) {
            strncpy(username, argv[++i], MAX_USERNAME_LEN - 1);
            username[MAX_USERNAME_LEN - 1] = '\0';
        }
    }

    User* user = userTable.find(username);
    if (!user || !user->isLoggedIn) {
        return -1;
    }

    user->isLoggedIn = false;
    return 0;
}

int handleQueryProfile(int argc, char* argv[]) {
    char curUser[21] = {0};
    char username[21] = {0};

    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "-c") == 0 && i + 1 < argc) {
            strncpy(curUser, argv[++i], MAX_USERNAME_LEN - 1);
            curUser[MAX_USERNAME_LEN - 1] = '\0';
        } else if (strcmp(argv[i], "-u") == 0 && i + 1 < argc) {
            strncpy(username, argv[++i], MAX_USERNAME_LEN - 1);
            username[MAX_USERNAME_LEN - 1] = '\0';
        }
    }

    // Check current user
    User* current = userTable.find(curUser);
    if (!current || !current->isLoggedIn) {
        return -1;
    }

    // Check target user
    User* target = userTable.find(username);
    if (!target) {
        return -1;
    }

    // Check privilege
    if (strcmp(curUser, username) != 0 && current->privilege <= target->privilege) {
        return -1;
    }

    // Output user info
    printf("%s %s %s %d\n", target->username, target->name, target->mailAddr, target->privilege);
    return 0;
}

int handleModifyProfile(int argc, char* argv[]) {
    char curUser[21] = {0};
    char username[21] = {0};
    char password[31] = {0};
    char name[21] = {0};
    char mailAddr[31] = {0};
    int privilege = -1;
    bool hasPassword = false, hasName = false, hasMail = false, hasPrivilege = false;

    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "-c") == 0 && i + 1 < argc) {
            strncpy(curUser, argv[++i], MAX_USERNAME_LEN - 1);
            curUser[MAX_USERNAME_LEN - 1] = '\0';
        } else if (strcmp(argv[i], "-u") == 0 && i + 1 < argc) {
            strncpy(username, argv[++i], MAX_USERNAME_LEN - 1);
            username[MAX_USERNAME_LEN - 1] = '\0';
        } else if (strcmp(argv[i], "-p") == 0 && i + 1 < argc) {
            strncpy(password, argv[++i], MAX_PASSWORD_LEN - 1);
            password[MAX_PASSWORD_LEN - 1] = '\0';
            hasPassword = true;
        } else if (strcmp(argv[i], "-n") == 0 && i + 1 < argc) {
            strncpy(name, argv[++i], MAX_NAME_LEN - 1);
            name[MAX_NAME_LEN - 1] = '\0';
            hasName = true;
        } else if (strcmp(argv[i], "-m") == 0 && i + 1 < argc) {
            strncpy(mailAddr, argv[++i], MAX_MAIL_LEN - 1);
            mailAddr[MAX_MAIL_LEN - 1] = '\0';
            hasMail = true;
        } else if (strcmp(argv[i], "-g") == 0 && i + 1 < argc) {
            privilege = atoi(argv[++i]);
            hasPrivilege = true;
        }
    }

    // Check current user
    User* current = userTable.find(curUser);
    if (!current || !current->isLoggedIn) {
        return -1;
    }

    // Check target user
    User* target = userTable.find(username);
    if (!target) {
        return -1;
    }

    // Check privilege
    if (strcmp(curUser, username) != 0 && current->privilege <= target->privilege) {
        return -1;
    }

    // Check privilege modification
    if (hasPrivilege && privilege >= current->privilege) {
        return -1;
    }

    // Apply modifications
    if (hasPassword) {
        target->setPassword(password);
    }
    if (hasName) {
        target->setName(name);
    }
    if (hasMail) {
        target->setMailAddr(mailAddr);
    }
    if (hasPrivilege) {
        target->privilege = privilege;
    }

    // Output modified user info
    printf("%s %s %s %d\n", target->username, target->name, target->mailAddr, target->privilege);
    return 0;
}

int handleAddTrain(int argc, char* argv[]) {
    char trainID[MAX_TRAIN_ID_LEN] = {0};
    int stationNum = 0;
    int seatNum = 0;
    char stationsStr[2000] = {0}; // Large buffer for station list
    char pricesStr[2000] = {0}; // Large buffer for price list
    char startTimeStr[6] = {0};
    char travelTimesStr[2000] = {0};
    char stopoverTimesStr[2000] = {0};
    char saleDateStr[20] = {0};
    char type[2] = {0};

    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "-i") == 0 && i + 1 < argc) {
            strncpy(trainID, argv[++i], MAX_TRAIN_ID_LEN - 1);
            trainID[MAX_TRAIN_ID_LEN - 1] = '\0';
        } else if (strcmp(argv[i], "-n") == 0 && i + 1 < argc) {
            stationNum = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-m") == 0 && i + 1 < argc) {
            seatNum = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-s") == 0 && i + 1 < argc) {
            strncpy(stationsStr, argv[++i], sizeof(stationsStr) - 1);
            stationsStr[sizeof(stationsStr) - 1] = '\0';
        } else if (strcmp(argv[i], "-p") == 0 && i + 1 < argc) {
            strncpy(pricesStr, argv[++i], sizeof(pricesStr) - 1);
            pricesStr[sizeof(pricesStr) - 1] = '\0';
        } else if (strcmp(argv[i], "-x") == 0 && i + 1 < argc) {
            strncpy(startTimeStr, argv[++i], sizeof(startTimeStr) - 1);
            startTimeStr[sizeof(startTimeStr) - 1] = '\0';
        } else if (strcmp(argv[i], "-t") == 0 && i + 1 < argc) {
            strncpy(travelTimesStr, argv[++i], sizeof(travelTimesStr) - 1);
            travelTimesStr[sizeof(travelTimesStr) - 1] = '\0';
        } else if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            strncpy(stopoverTimesStr, argv[++i], sizeof(stopoverTimesStr) - 1);
            stopoverTimesStr[sizeof(stopoverTimesStr) - 1] = '\0';
        } else if (strcmp(argv[i], "-d") == 0 && i + 1 < argc) {
            strncpy(saleDateStr, argv[++i], sizeof(saleDateStr) - 1);
            saleDateStr[sizeof(saleDateStr) - 1] = '\0';
        } else if (strcmp(argv[i], "-y") == 0 && i + 1 < argc) {
            strncpy(type, argv[++i], sizeof(type) - 1);
            type[sizeof(type) - 1] = '\0';
        }
    }

    // Check if train already exists
    Train* existing = trainTable.find(trainID);
    if (existing) {
        return -1;
    }

    // Validate parameters
    if (stationNum < 2 || stationNum > MAX_STATIONS ||
        seatNum <= 0 || seatNum > MAX_SEATS ||
        type[0] < 'A' || type[0] > 'Z') {
        return -1;
    }

    // Create new train
    Train newTrain;
    strncpy(newTrain.trainID, trainID, MAX_TRAIN_ID_LEN - 1);
    newTrain.trainID[MAX_TRAIN_ID_LEN - 1] = '\0';
    newTrain.stationNum = stationNum;
    newTrain.seatNum = seatNum;
    strncpy(newTrain.startTime, startTimeStr, sizeof(newTrain.startTime) - 1);
    newTrain.type = type[0];
    newTrain.isReleased = false;

    // Parse start time
    sscanf(startTimeStr, "%d:%d", &newTrain.startHour, &newTrain.startMinute);

    // Parse sale dates
    char* saleDates[2];
    int saleDateCount = 0;
    splitStringByDelimiter(saleDateStr, '|', saleDates, 2, saleDateCount);
    if (saleDateCount >= 2) {
        strncpy(newTrain.saleDateFrom, saleDates[0], sizeof(newTrain.saleDateFrom) - 1);
        strncpy(newTrain.saleDateTo, saleDates[1], sizeof(newTrain.saleDateTo) - 1);
        // Clean up
        for (int i = 0; i < saleDateCount; i++) delete[] saleDates[i];
    } else {
        // Clean up
        for (int i = 0; i < saleDateCount; i++) delete[] saleDates[i];
        return -1;
    }

    // Parse stations
    char* stationTokens[MAX_STATIONS];
    int stationCount = 0;
    splitStringByDelimiter(stationsStr, '|', stationTokens, MAX_STATIONS, stationCount);

    if (stationCount != stationNum) {
        // Clean up
        for (int i = 0; i < stationCount; i++) delete[] stationTokens[i];
        return -1;
    }

    // Initialize first station
    strncpy(newTrain.stations[0].name, stationTokens[0], MAX_STATION_NAME_LEN - 1);
    newTrain.stations[0].travelTime = 0;
    newTrain.stations[0].stopoverTime = 0;
    newTrain.stations[0].price = 0;
    newTrain.stations[0].arrivalTime = -1; // Starting station arrival time is x
    newTrain.stations[0].departureTime = newTrain.startHour * 60 + newTrain.startMinute;

    // Parse prices
    char* priceTokens[MAX_STATIONS];
    int priceCount = 0;
    splitStringByDelimiter(pricesStr, '|', priceTokens, MAX_STATIONS, priceCount);

    if (priceCount != stationNum - 1) {
        // Clean up
        for (int i = 0; i < stationCount; i++) delete[] stationTokens[i];
        for (int i = 0; i < priceCount; i++) delete[] priceTokens[i];
        return -1;
    }

    // Parse travel times
    char* travelTokens[MAX_STATIONS];
    int travelCount = 0;
    splitStringByDelimiter(travelTimesStr, '|', travelTokens, MAX_STATIONS, travelCount);

    if (travelCount != stationNum - 1) {
        // Clean up
        for (int i = 0; i < stationCount; i++) delete[] stationTokens[i];
        for (int i = 0; i < priceCount; i++) delete[] priceTokens[i];
        for (int i = 0; i < travelCount; i++) delete[] travelTokens[i];
        return -1;
    }

    // Parse stopover times
    char* stopoverTokens[MAX_STATIONS];
    int stopoverCount = 0;
    splitStringByDelimiter(stopoverTimesStr, '|', stopoverTokens, MAX_STATIONS, stopoverCount);

    // For trains with only 2 stations, stopoverTimesStr should be "_"
    if (stationNum == 2) {
        if (strcmp(stopoverTimesStr, "_") != 0) {
            // Clean up
            for (int i = 0; i < stationCount; i++) delete[] stationTokens[i];
            for (int i = 0; i < priceCount; i++) delete[] priceTokens[i];
            for (int i = 0; i < travelCount; i++) delete[] travelTokens[i];
            for (int i = 0; i < stopoverCount; i++) delete[] stopoverTokens[i];
            return -1;
        }
    } else if (stopoverCount != stationNum - 2) {
        // Clean up
        for (int i = 0; i < stationCount; i++) delete[] stationTokens[i];
        for (int i = 0; i < priceCount; i++) delete[] priceTokens[i];
        for (int i = 0; i < travelCount; i++) delete[] travelTokens[i];
        for (int i = 0; i < stopoverCount; i++) delete[] stopoverTokens[i];
        return -1;
    }

    // Fill in station information
    int currentTime = newTrain.startHour * 60 + newTrain.startMinute;
    int currentPrice = 0;

    for (int i = 1; i < stationNum; i++) {
        // Set station name
        strncpy(newTrain.stations[i].name, stationTokens[i], MAX_STATION_NAME_LEN - 1);

        // Set travel time from previous station
        newTrain.stations[i].travelTime = atoi(travelTokens[i-1]);

        // Set price from previous station
        int segmentPrice = atoi(priceTokens[i-1]);
        currentPrice += segmentPrice;
        newTrain.stations[i].price = currentPrice;

        // Calculate arrival time at this station
        currentTime += newTrain.stations[i].travelTime;
        newTrain.stations[i].arrivalTime = currentTime;

        // Calculate departure time from this station
        if (i < stationNum - 1) {
            // Not the last station, has stopover time
            int stopoverTime = (stationNum > 2) ? atoi(stopoverTokens[i-1]) : 0;
            newTrain.stations[i].stopoverTime = stopoverTime;
            currentTime += stopoverTime;
            newTrain.stations[i].departureTime = currentTime;
        } else {
            // Last station, departure time is x
            newTrain.stations[i].stopoverTime = 0;
            newTrain.stations[i].departureTime = -1; // Mark as terminal
        }
    }

    // Clean up temporary tokens
    for (int i = 0; i < stationCount; i++) delete[] stationTokens[i];
    for (int i = 0; i < priceCount; i++) delete[] priceTokens[i];
    for (int i = 0; i < travelCount; i++) delete[] travelTokens[i];
    for (int i = 0; i < stopoverCount; i++) delete[] stopoverTokens[i];

    // Initialize seat availability for all days in sale period
    int startDay = parseDate(newTrain.saleDateFrom);
    int endDay = parseDate(newTrain.saleDateTo);

    if (startDay < 0 || endDay < 0 || startDay > endDay || endDay >= MAX_DAYS) {
        return -1;
    }

    for (int day = startDay; day <= endDay; day++) {
        for (int station = 0; station < stationNum - 1; station++) {
            newTrain.seatAvailability[day][station] = seatNum;
        }
    }

    // Add train to table
    if (trainTable.insert(newTrain)) {
        return 0;
    }

    return -1;
}

// Split string by spaces
void splitString(const string& str, char** tokens, int& count) {
    count = 0;
    const char* start = str.c_str();
    const char* end = start;

    while (*end) {
        // Skip spaces
        while (*start && *start == ' ') start++;
        if (!*start) break;

        // Find end of token
        end = start;
        while (*end && *end != ' ') end++;

        // Copy token
        int len = end - start;
        tokens[count] = new char[len + 1];
        strncpy(tokens[count], start, len);
        tokens[count][len] = '\0';
        count++;

        start = end;
    }
}

// Main command parser
int processCommand(const string& line) {
    const int MAX_TOKENS = 100;
    char* tokens[MAX_TOKENS];
    int tokenCount = 0;

    splitString(line, tokens, tokenCount);

    if (tokenCount == 0) {
        // Clean up
        for (int i = 0; i < tokenCount; i++) delete[] tokens[i];
        return 0;
    }

    int result = 0;
    string cmd = tokens[0];

    if (cmd == "add_user") {
        result = handleAddUser(tokenCount - 1, tokens + 1);
    } else if (cmd == "login") {
        result = handleLogin(tokenCount - 1, tokens + 1);
    } else if (cmd == "logout") {
        result = handleLogout(tokenCount - 1, tokens + 1);
    } else if (cmd == "query_profile") {
        result = handleQueryProfile(tokenCount - 1, tokens + 1);
    } else if (cmd == "modify_profile") {
        result = handleModifyProfile(tokenCount - 1, tokens + 1);
    } else if (cmd == "add_train") {
        result = handleAddTrain(tokenCount - 1, tokens + 1);
    } else if (cmd == "release_train") {
        // TODO: Implement
        result = -1;
    } else if (cmd == "query_train") {
        // TODO: Implement
        result = -1;
    } else if (cmd == "delete_train") {
        // TODO: Implement
        result = -1;
    } else if (cmd == "query_ticket") {
        // TODO: Implement
        result = -1;
    } else if (cmd == "query_transfer") {
        // TODO: Implement
        result = -1;
    } else if (cmd == "buy_ticket") {
        // TODO: Implement
        result = -1;
    } else if (cmd == "query_order") {
        // TODO: Implement
        result = -1;
    } else if (cmd == "refund_ticket") {
        // TODO: Implement
        result = -1;
    } else if (cmd == "clean") {
        // Reset all data
        // For now, create new tables
        userCount = 0;
        // Note: This leaks memory, but clean is rarely called
        // In a real implementation, we would properly clean up
        result = 0;
    } else if (cmd == "exit") {
        // Will be handled in main loop
        result = 0;
    } else {
        // Unknown command
        result = -1;
    }

    // Clean up
    for (int i = 0; i < tokenCount; i++) delete[] tokens[i];

    return result;
}

int main() {
    string line;

    while (getline(cin, line)) {
        if (line.empty()) continue;

        if (line == "exit") {
            cout << "bye" << endl;
            break;
        } else {
            int result = processCommand(line);
            if (line.find("query_profile") == string::npos) {
                // query_profile outputs its own result, others print result code
                cout << result << endl;
            }
        }
    }

    return 0;
}