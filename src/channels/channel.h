#include <cstdint>
#include <string>
#include <unordered_set>

typedef uint64_t UserId;

class Channel {
public:
  Channel(std::string name);
  ~Channel();

private:
  std::string name;
  std::unordered_set<UserId> users;
};
