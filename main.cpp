#include <iostream>
#include <fstream>
#include <vector>
#include <cstdint>
#include <string>
using namespace std;

vector<uint8_t> LoadFile(const char* path)
{
    vector<uint8_t> result;

    if (path && *path)
    {
        ifstream fin(path, ifstream::binary);

        if (fin)
        {
            fin.seekg(0, ios::end);
            result.resize(fin.tellg());
            fin.seekg(0, ios::beg);
            fin.read((char*)result.data(), result.size());
            fin.close();
        }
    }

    return move(result);
}

template<typename T>
inline const T Get(const void* target)
{
    return *reinterpret_cast<const T*>(target);
}

template<typename T>
const T Read(const uint8_t*& target)
{
    const T result = Get<T>(target);
    target += sizeof(T);
    return result;
}

template<typename T>
const T EndianSwap(const T value)
{
    const uint8_t* oldBytes = reinterpret_cast<const uint8_t*>(&value);
    uint8_t newBytes[sizeof(T)];

    const size_t LastIndex = sizeof(T) - 1;
    for (size_t i = 0; i < sizeof(T); ++i)
        newBytes[i] = oldBytes[LastIndex - i];

    return *reinterpret_cast<const T*>(newBytes);
}

string ReadString(const uint8_t*& data)
{
    size_t length = EndianSwap(Get<uint16_t>(data));
    const char* block = (const char*)(data + 2);
    data += length + 2;
    return string(block, length);
}

void Dump(const vector<uint8_t>& data, ostream& out)
{
    const char* worldLabels[] = {
        "WorldX",
        "WorldY",
        "WorldXa",
        "WorldYa",
        "WorldZa"
    };

    const uint8_t* current = data.data();
    for (int i = 0; i < 5; ++i)
    {
        out << worldLabels[i]
            << " : "
            << EndianSwap(Read<int>(current))
            << '\n';
    }

    current += 5; // unknown block

    const char* positionLabels[] = {
        "x offset",
        "y offset",
        "X",
        "Y",
        "Z"
    };

    for (int i = 0; i < 5; ++i)
    {
        out << positionLabels[i]
            << " : "
            << Read<float>(current)
            << '\n';
    }

    out << "Player Direction : " << EndianSwap(Read<int>(current)) << '\n';

    int controlByte = Read<uint8_t>(current);
    out << "Control Byte : " << controlByte << '\n';

    if (controlByte)
    {
        auto playerTableLength = EndianSwap(Read<int>(current));
        out << "Player Table Length : " << playerTableLength << '\n';

        for (decltype(playerTableLength) i = 0; i < playerTableLength; ++i)
        {
            int type = Read<uint8_t>(current);
            out << "  Type : " << type << '\n';

            if (type == 0)
            {
                string name = ReadString(current);

                out << "    Name : " << name << '\n';

                // I'm probably not doing this step correctly.
                // Every string was followed by a 01, so I improvised.
                int subtype = Read<uint8_t>(current);

                if (subtype == 1)
                {
                    out << "    Value : "
                        //<< Read<int64_t>(current)
                        << EndianSwap(Read<double>(current))
                        << '\n';
                }
            }
        }
    }

    controlByte = Read<uint8_t>(current);
    out << "Control Byte : " << controlByte << '\n';

    if (controlByte)
    {
        auto playerId = EndianSwap(Read<int>(current));
        out << "  Player ID : " << playerId << '\n';

        const char* stringLabels[] = {
            "Forename",
            "Surname",
            "Legs",
            "Torso",
            "Head",
            "Top",
            "Bottoms",
            "Shoes",
            "Shoespal",
            "Bottomspal",
            "Toppal",
            "Skinpal",
            "Hair"
        };

        for (int i = 0; i < 13; ++i)
        {
            out << "  "
                << stringLabels[i]
                << " : "
                << ReadString(current) << '\n';
        }

        out << "  Grender : " << EndianSwap(Read<int>(current)) << '\n';
        out << "  Profession : " << ReadString(current) << '\n';

        const char* floatLabels[] = {
            "Hair Color R",
            "Hair Color G",
            "Hair Color B",
            "Top Color R",
            "Top Color G",
            "Top Color B",
            "Trouser Color R",
            "Trouser Color G",
            "Trouser Color B"
        };

        for (int i = 0; i < 9; ++i)
        {
            out << "  "
                << floatLabels[i]
                << " : "
                //<< Read<float>(current)
                << EndianSwap(Read<float>(current))
                << '\n';
        }

        //current += 16; // Not sure what's missing.
        for (int i = 0; i < 4; ++i)
        {
            out << "  Unknown : "
                << EndianSwap(Read<float>(current))
                << '\n';
        }
    }

    out << "Inventory Type : " << ReadString(current) << '\n';
    out << "InvExplored : " << (int)Read<uint8_t>(current) << '\n';

    int inventoryCount = EndianSwap(Read<int16_t>(current));
    out << "InvCount : " << inventoryCount << '\n';

    for (int i = 0; i < inventoryCount; ++i)
    {
        out << "  Item : " << ReadString(current) << '\n';
        out << "  Uses? : " << EndianSwap(Read<int>(current)) << '\n';
        break; // Avoid bad things for now. XD
    }
}

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        cerr << "Y U NO SPECIFY INPUT FILES\n";
        return 1;
    }

    for (int i = 1; i < argc; ++i)
    {
        vector<uint8_t> data = LoadFile(argv[i]);

        if (data.size() > 0)
        {
            Dump(data, cout);
        }
        else
        {
            cerr << "failed: " << argv[i] << '\n';
        }
    }

    return 0;
}
