#include <string>

// Declare the function signature correctly
int graphrun(const std::string& filename);

int main(int argc, char* argv[]) {
    std::string filename = (argc > 1) ? argv[1] : "test.mid";
    return graphrun(filename);
}