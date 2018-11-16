//============================================================================
// Name        : SudokuSolver.cpp
// Author      : Jeff Hancock
//               https://www.linkedin.com/in/jeffreythancock/
// Copyright   : Carte blanche.  Plagiarize at will.
// Description : A programming that checks to see if a given solution to a
//               Sudoku puzzle is valid, and also solves an incomplete puzzle.
//============================================================================

#include <iostream>
#include <fstream>
#include <sstream>
#include <set>
using namespace std;

enum class RowOrCol {
    ROW,
    COL
};

class SudokuPuzzle {

public:

    // Constructor taking a 2D array of ints
    SudokuPuzzle(const int potentialSolution[9][9]);

    // Constructor taking a filename of a CSV file, with nine lines of ints,
    // nine per line, each line representing a row of the puzzle board.
    SudokuPuzzle(const string &fn);

    ~SudokuPuzzle();

    // Check the given puzzle to see if it is a valid solution.
    // Return value:
    //    true - The puzzle contains a valid solution.
    //    false - The puzzle does NOT contain a valid solution.
    bool isSolutionValid(const bool verbose = false) const;

    // Solve an incomplete puzzle.
    // Return value:
    //    true - puzzle was solved
    //    false - puzzle was not solved.  Either there is no valid solution or
    //            the algorithm is insufficient (defective).
    bool solve(const bool verbose = false);

    // Get the value of specified location on the board.  Zero based indexing.
    int getValue(const int row, const int col) const;

    // Set the value of the specified location on the board to the specified value.
    void setValue(const int row, const int col, const int value);

private:
    int board[9][9];

    // For each blank spot on the board, a list of valid values for the current state of the
    // puzzle.  A nullptr for each spot that already has its value set (non-zero).
    set<int>* possibilities[9][9];

    int minPossibilities;
    int minRow;
    int minCol;

    // Not really part of the state of the puzzle.  Set by isSolutionValid.
    // Used by several of the ...Ok methods.
    mutable bool verbose = false;

    bool isRowOrColOk(const int row, const RowOrCol which = RowOrCol::ROW) const;

    bool isSubmatrixOk(const int startRowIdx, const int startColIdx) const;

    bool areSubmatricesOk() const;

    void print() const;

    bool wouldWork(const int value, const int atRow, const int atCol);

    void setAllPossibilities();

    int trimPossibilities();

    void listPossibilities() const;

};

SudokuPuzzle::SudokuPuzzle(const int potentialSolution[9][9]) {
    // Initialize these three to something invalid, and in the case of
    // minPossibilites, the number to beat (easy!)
    minPossibilities = 10;
    minRow = 10;
    minCol = 10;
    // Set the values of the board
    for (int row=0; row < 9; row++) {
        for (int col=0; col < 9; col++) {
            board[row][col] = potentialSolution[row][col];
        }
    }

    // Initialize possibilities to nullptr
    for (int row=0; row < 9; row++) {
        for (int col=0; col < 9; col++) {
            possibilities[row][col] = nullptr;
        }
    }
}

SudokuPuzzle::SudokuPuzzle(const string &fn) {
    // Initialize these three to something invalid, and in the case of
    // minPossibilites, the number to beat (easy!)
    minPossibilities = 10;
    minRow = 10;
    minCol = 10;
    try {
        ifstream infile(fn);
        if (infile.good()) { // check for success opening file
            string line;
            int row = 0, col = 0;
            // Read each line
            while (getline(infile, line)) {
                // Use getline on a stringstream to parse values between commas
                string value;
                stringstream lineStream(line);
                while (getline(lineStream, value, ',')) {
                    // Accept a space, empty string, or 0 as not filled in yet, but represent
                    // it on the board as a zero.
                    if (value == " " or value == "" or value == "0") {
                        board[row][col] = 0;
                    }
                    else {
                        board[row][col] = stoi(value);
                    }
                    col++;
                }
                // Done with the row.  Increment row and reset col to zero.
                row++;
                col = 0;
            }
        }
        else {
            cerr << "Failed to open file: " << fn << endl;
        }
    }
    catch (std::exception& e) {
        cerr << "Exception in constructor reading file.";
    }

    // Initialize possibilities to nullptr
    for (int row=0; row < 9; row++) {
        for (int col=0; col < 9; col++) {
            possibilities[row][col] = nullptr;
        }
    }

    print(); // for debug...
}

bool SudokuPuzzle::solve(const bool verbose) {
    this->verbose = verbose;
    // Initialize the list of possible values for each blank space to 1 through 9.  These
    // lists will then be trimmed be checking to see which of the values are actually OK,
    // and which aren't.
    setAllPossibilities();
    // Keep track of the number of blank spots in the previous iteration, so that we can
    // detect no progress.  Initialize it to more than possible.
    int prevNumBlank = 50;
    while (true) {
        int numBlank = trimPossibilities();
        // Check for a return of -1, which indicates the puzzle can't be solved.
        if (numBlank == -1) {
            return false;  // Puzzle can't be solved.
        }
        // Check for the puzzle now being solved (no blanks left)
        if (numBlank == 0) {
            cout << "Solution:" << endl;
            print();
            return true;
        }
        if (numBlank == prevNumBlank) {

            // First, let's try printing what the next space with the lowest number of possibilities is, and
            // what that number is.
            if (verbose) cout << "MinPossibilites: " << minPossibilities << ", minRow: " << minRow << ", minCol: " << minCol << endl;

            // Iterate over that set of possibilities, creating a new puzzle with that value, and
            // try to solve it (recursive call).
            for (set<int>::iterator it = possibilities[minRow][minCol]->begin(); it != possibilities[minRow][minCol]->end(); it++) {
                if (verbose) cout << "Creating sub-puzzle for value: " << *it << endl;

                SudokuPuzzle* subPuzzle = new SudokuPuzzle(board); // based on current puzzle
                subPuzzle->setValue(minRow, minCol, *it); // but with this value set to see how it works out

                // If solved, copy values of the sub-puzzle into this one and return true.
                if (subPuzzle->solve()) {
                    for (int row=0; row < 9; row++) {
                        for (int col=0; col < 9; col++) {
                            // For all blanks in this puzzle, copy values from sub-puzzle
                            if (board[row][col] == 0) board[row][col] = subPuzzle->getValue(row, col);
                        }
                    }
                    // Delete the sub-puzzle
                    delete subPuzzle;
                    subPuzzle = nullptr;
                    return true;
                }
                // That value didn't work out.  Delete the sub-puzzle.
                delete subPuzzle;
                subPuzzle = nullptr;
            }
            // If survive loop without a solution, return false
            return false; // stuck
        }
        prevNumBlank = numBlank;
    } // end while loop
}

int SudokuPuzzle::getValue(const int row, const int col) const {
    return board[row][col];
}

void SudokuPuzzle::setValue(const int row, const int col, const int value) {
    board[row][col] = value;
}

SudokuPuzzle::~SudokuPuzzle() {
    for (int row=0; row < 9; row++) {
        for (int col=0; col < 9; col++) {
            if (possibilities[row][col]) {
                delete possibilities[row][col];
                possibilities[row][col] = nullptr;
            }
        }
    }
}

// Check the row of the puzzle to make sure each entry in the row is both
// unique and a valid value (int 0 - 9 only, zero being used to represent blank),
//                        OR
// Check the column of the puzzle for uniqueness.
//
// Return value:
//    true - The row (or col) is OK
//    false - The row (or col) is NOT OK.  There is either an invalid value  or a
//            duplicate within the row (or col).
bool SudokuPuzzle::isRowOrColOk(const int row, const RowOrCol which) const {
    // Check row for valid and non-repeating values.  The non-repeating
    // aspect will be done by using a set and adding the values to the
    // set, checking for a return code indicating the value to be added
    // already exists.
    set<int> values;

    // Check the row to make sure it doesn't have any duplicates
    values.clear();
    for (int col = 0; col < 9; col++) {
        int value;

        // If we're checking a row, grab the value using indices as expected, but
        // if we're checking a column, switch the indices.  This lets us check
        // either with one method instead of two nearly identical methods.
        if (which == RowOrCol::ROW) value = board[row][col];
        else value = board[col][row];

        // Check uniqueness
        if (value != 0) {
            auto rc = values.insert(value);
            if (!rc.second) { // If insert not successful due to value already being in set
                if (verbose) {
                    // Do the appropriate message depending on if we're checking rows or columns
                    if (which == RowOrCol::ROW) cout << "row " << row << " has a repeat value: " << value << endl;
                    else cout << "col " << row << " has a repeat value: " << value << endl;
                    // Yes, we really want to use the row variable in the line above, since in the
                    // case of checking columns, the meanings of the row and col variables are reversed.
                }
                return false;
            }
        }
    }
    if (verbose) {
        // Do the appropriate message depending on if we're checking rows or columns
        if (which == RowOrCol::ROW) cout << "row " << row << " is OK." << endl;
        else cout << "col " << row << " is OK." << endl;
        // Yes, we really want to use the row variable in the line above, since in the
        // case of checking columns, the meanings of the row and col variables are reversed.
    }
    return true;
}

// Check the specified submatrix of the puzzle to make sure each entry in the submatrix is unique.
// Return value:
//    true - The submatrix is OK
//    false - The submatrix are NOT OK.  There is a duplicate within the submatrix.
bool SudokuPuzzle::isSubmatrixOk(const int startRowIdx, const int startColIdx) const {
    set<int> values;
    int stopRowIdx = startRowIdx + 3; // Stop indices are exclusive
    int stopColIdx = startColIdx + 3;
    for (int row = startRowIdx; row < stopRowIdx; row++) {
        for (int col = startColIdx; col < stopColIdx; col++) {
            int value = board[row][col];
            if (value != 0) {
                auto rc = values.insert(value);
                if (!rc.second) {
                    if (verbose) cout << "Submatrix with starting row " << startRowIdx <<
                        " and starting column " << startColIdx << " has a repeat value: " << value << endl;
                    return false;
                }
            }
        }
    }
    if (verbose) cout << "Submatrix with starting row " << startRowIdx << " and starting column " <<
        startColIdx << " is OK." << endl;
    return true;
}

// Check the submatrices of the puzzle to make sure each entry in each submatrix is unique.
// Return value:
//    true - The submatrices are OK
//    false - The submatrices are NOT OK.  There is a duplicate within one or more submatrix.
bool SudokuPuzzle::areSubmatricesOk() const {
    // Check sub-matrices (the 9 3x3 squares)
    for (int startRow = 0; startRow < 7; startRow += 3) {
        for (int startCol = 0; startCol < 7; startCol += 3) {
            if (!isSubmatrixOk(startRow, startCol))
                return false;
        }
    }
    return true;
}


bool SudokuPuzzle::isSolutionValid(bool verbose) const {
    this->verbose = verbose;
    // Check rows
    for (int row = 0; row < 9; row++) {
        if (!isRowOrColOk(row)) // ROW is default
            return false;
    }
    // Check columns
    for (int col = 0; col < 9; col++) {
        if (!isRowOrColOk(col, RowOrCol::COL))
            return false;
    }
    return areSubmatricesOk();
}

// Print the contents of the puzzle to stdout.
void SudokuPuzzle::print() const {
    cout << "-------------------------------------" << endl;
    for (int row=0; row < 9; row++) {
        cout << "|";
        for (int col=0; col < 9; col++) {
            if (board[row][col] == 0) cout << "   |";
            else cout << " " << board[row][col] << " |";
        }
        cout << endl;
        cout << "-------------------------------------" << endl;
    }
}

// See if the given value would be valid in the given spot.
// Return value:
//    true - it would work
//    false - it wouldn't
bool SudokuPuzzle::wouldWork(const int value, const int atRow, const int atCol) {
    board[atRow][atCol] = value;
    if (!isRowOrColOk(atRow)) {
        board[atRow][atCol] = 0;
        return false;
    }
    if (!isRowOrColOk(atCol, RowOrCol::COL)) {
        board[atRow][atCol] = 0;
        return false;
    }
    if (isSubmatrixOk(3 * (atRow / 3), 3 * (atCol / 3))) {
        board[atRow][atCol] = 0;
        return true;
    }
    else {
        board[atRow][atCol] = 0;
        return false;
    }
}

// For every blank space on the board, set the list of possibilities to all
// numbers 1 - 9.  The list will then be trimmed of those that don't work.
void SudokuPuzzle::setAllPossibilities() {
    for (int row=0; row < 9; row++) {
        for (int col=0; col < 9; col++) {
            if (board[row][col] == 0) {
                possibilities[row][col] = new set<int> {1, 2, 3, 4, 5, 6, 7, 8, 9};
            }
            else {
                possibilities[row][col] = nullptr;
            }
        }
    }
}

// Trim the list of possibilities for each blank space to those that would actually
// work.  In those cases where the list ends up being length 1, we know the proper
// value for that spot, so go ahead and set it.
// Return value:
//    The number of spots that are still blank (should be between 0 and 81)
int SudokuPuzzle::trimPossibilities() {
    // Initialize these three to something invalid, and in the case of
    // minPossibilites, the number to beat (easy!)
    minPossibilities = 10;
    minRow = 10;
    minCol = 10;
    if (verbose) print();
    int numBlank = 0;

    for (int row=0; row < 9; row++) {
        for (int col=0; col < 9; col++) {
            if (possibilities[row][col]) {
                // Iterate over the set of possible numbers, and check each to see if it would actually work.
                for (auto it = possibilities[row][col]->begin(); it != possibilities[row][col]->end();) {
                    if (!wouldWork(*it, row, col)) {
                        // Remove it, since it wouldn't work
                        it = possibilities[row][col]->erase(it);
                    }
                    else it++;
                }
                // If verbose, List the possibilities
                if (verbose) {
                    cout << "Number of possibilities for row " << row << ", col " << col << ": " <<
                        possibilities[row][col]->size() << " (";
                    for (auto it = possibilities[row][col]->begin(); it != possibilities[row][col]->end(); it++) {
                        cout << *it << " ";
                    }
                    cout << ")" << endl;
                }
                if (possibilities[row][col]->size() == 0) {
                    // There are no possible values for this spot.  Puzzle can't be solved.
                    return -1;
                }
                // If there is only one possible value that works for this space, set it to that
                // value and continue!
                if (possibilities[row][col]->size() == 1) {
                    if (verbose) cout << "***** Gonna set it to " << *(possibilities[row][col]->begin()) << endl;
                    board[row][col] = *(possibilities[row][col]->begin());
                    delete possibilities[row][col];
                    possibilities[row][col] = nullptr;
                }
                else {
                    numBlank++; // Count this spot that is still blank

                    // See if this list is shorter than the shortest so far, and if so, keep it as
                    // the new shortest.
                    if (int(possibilities[row][col]->size()) < minPossibilities) {
                        minPossibilities = possibilities[row][col]->size();
                        minRow = row;
                        minCol = col;
                    }
                }
            }
        }
    }
    if (verbose) cout << "trimPossibilites returning " << numBlank << endl;
    return numBlank;
}

// Print out the list of possibilities for each spot
void SudokuPuzzle::listPossibilities() const {
    for (int row=0; row < 9; row++) {
        for (int col=0; col < 9; col++) {
            if (possibilities[row][col] != nullptr) {
                cout << "For row " << row << ", col " << col << ", possibilities are: ";
                for (auto it = possibilities[row][col]->begin(); it != possibilities[row][col]->end(); it++) {
                    cout << *it << " ";
                }
                cout << endl;
            }
        }
    }
}

int main() {

    {
        // A solution to be tested that passes the row and column
        // tests but not the submatrices test.
        int puzzle1[9][9] = { { 1, 2, 3, 4, 5, 6, 7, 8, 9 },
                              { 2, 3, 4, 5, 6, 7, 8, 9, 1 },
                              { 3, 4, 5, 6, 7, 8, 9, 1, 2 },
                              { 4, 5, 6, 7, 8, 9, 1, 2, 3 },
                              { 5, 6, 7, 8, 9, 1, 2, 3, 4 },
                              { 6, 7, 8, 9, 1, 2, 3, 4, 5 },
                              { 7, 8, 9, 1, 2, 3, 4, 5, 6 },
                              { 8, 9, 1, 2, 3, 4, 5, 6, 7 },
                              { 9, 1, 2, 3, 4, 5, 6, 7, 8 } };

        SudokuPuzzle sp(puzzle1);
        cout << "Solution to puzzle1 is " << (sp.isSolutionValid(true) ? "" : "NOT ") << "valid." << endl;
    }

    {
        // A supposedly solved one from google images, but not (row idx 6 repeats 3)
        int puzzle2[9][9] = { { 2, 9, 5, 7, 4, 3, 8, 6, 1 },
                          { 4, 3, 1, 8, 6, 5, 9, 2, 7 },
                          { 8, 7, 6, 1, 9, 2, 5, 4, 3 },
                          { 3, 8, 7, 4, 5, 9, 2, 1, 6 },
                          { 6, 1, 2, 3, 8, 7, 4, 9, 5 },
                          { 5, 4, 9, 2, 1, 6, 7, 3, 8 },
                          { 7, 6, 3, 5, 3, 4, 1, 8, 9 },
                          { 9, 2, 8, 6, 7, 1, 3, 5, 4 },
                          { 1, 5, 4, 9, 3, 8, 6, 7, 2 } };

        SudokuPuzzle sp(puzzle2);
        cout << "Solution to puzzle2 is " << (sp.isSolutionValid(true) ? "" : "NOT ") << "valid." << endl;

    }

    {
        // A truly solved one, also from google images.
        int puzzle3[9][9] = { { 8, 2, 7, 1, 5, 4, 3, 9, 6 },
                          { 9, 6, 5, 3, 2, 7, 1, 4, 8 },
                          { 3, 4, 1, 6, 8, 9, 7, 5, 2 },
                          { 5, 9, 3, 4, 6, 8, 2, 7, 1 },
                          { 4, 7, 2, 5, 1, 3, 6, 8, 9 },
                          { 6, 1, 8, 9, 7, 2, 4, 3, 5 },
                          { 7, 8, 6, 2, 3, 5, 9, 1, 4 },
                          { 1, 5, 4, 7, 9, 6, 8, 2, 3 },
                          { 2, 3, 9, 8, 4, 1, 5, 6, 7 } };

        SudokuPuzzle sp(puzzle3);
        cout << "Solution to puzzle3 is " << (sp.isSolutionValid() ? "" : "NOT ") << "valid." << endl;
    }

    {
        // An almost OK one, but with column failures.
        int puzzle4[9][9] = { { 8, 2, 7, 1, 5, 4, 3, 9, 6 },
                          { 9, 6, 5, 3, 2, 7, 1, 4, 8 },
                          { 3, 4, 1, 6, 8, 9, 7, 5, 2 },
                          { 5, 9, 3, 4, 6, 8, 2, 7, 1 },
                          { 4, 7, 2, 5, 1, 3, 6, 8, 9 },
                          { 6, 1, 8, 9, 7, 2, 4, 3, 5 },
                          { 7, 8, 6, 2, 3, 5, 9, 1, 4 },
                          { 1, 5, 4, 7, 9, 6, 8, 2, 3 },
                          { 2, 3, 9, 8, 4, 1, 5, 7, 6 } };

        SudokuPuzzle sp(puzzle4);
        cout << "Solution to puzzle4 is " << (sp.isSolutionValid(true) ? "" : "NOT ") << "valid." << endl;
    }

    {
        SudokuPuzzle sp("solvedPuzzle.txt");
        cout << "Solution to solvedPuzzle is " << (sp.isSolutionValid() ? "" : "NOT ") << "valid." << endl;
    }

    {
        SudokuPuzzle sp("easyPuzzle.txt");
        cout << "So far easyPuzzle is " << (sp.isSolutionValid() ? "" : "NOT ") << "valid." << endl;
        sp.solve();
    }

    {
        SudokuPuzzle sp("hardPuzzle.txt");
        cout << "So far hardPuzzle is " << (sp.isSolutionValid() ? "" : "NOT ") << "valid." << endl;
        sp.solve();
        cout << "Computed solution to hardPuzzle is " << (sp.isSolutionValid() ? "" : "NOT ") << "valid." << endl;
    }

    return 0;
}
