#include <cstddef>
#include <cstdint>
#include <cstring>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <system_error>
#include <vector>

namespace
{

constexpr std::uint32_t spirv_magic = 0x07230203U;

[[nodiscard]] std::vector<char> load_binary(const std::filesystem::path& path)
{
    std::error_code error;

    const std::uintmax_t byte_count = std::filesystem::file_size(path, error);

    if (error)
    {
        throw std::runtime_error{"cannot query shader binary: " + path.string()};
    }

    if (byte_count < 5U * sizeof(std::uint32_t) || byte_count % sizeof(std::uint32_t) != 0)
    {
        throw std::runtime_error{"invalid SPIR-V binary size: " + path.string()};
    }

    const auto maximum_stream_size =
        static_cast<std::uintmax_t>(std::numeric_limits<std::streamsize>::max());

    const auto maximum_container_size =
        static_cast<std::uintmax_t>(std::numeric_limits<std::size_t>::max());

    if (byte_count > maximum_stream_size || byte_count > maximum_container_size)
    {
        throw std::runtime_error{"shader binary is too large: " + path.string()};
    }

    std::vector<char> bytes(static_cast<std::size_t>(byte_count));

    std::ifstream stream{path, std::ios::binary};

    if (!stream)
    {
        throw std::runtime_error{"cannot open shader binary: " + path.string()};
    }

    stream.read(bytes.data(), static_cast<std::streamsize>(bytes.size()));

    if (!stream)
    {
        throw std::runtime_error{"cannot read shader binary: " + path.string()};
    }

    return bytes;
}

[[nodiscard]] std::uint32_t read_word(const std::vector<char>& bytes, std::size_t word_index)
{
    const std::size_t offset = word_index * sizeof(std::uint32_t);

    if (offset + sizeof(std::uint32_t) > bytes.size())
    {
        throw std::out_of_range{"SPIR-V word index is out of range"};
    }

    std::uint32_t value = 0;

    std::memcpy(&value, bytes.data() + offset, sizeof(value));

    return value;
}

void validate_spirv(const std::vector<char>& bytes, const std::filesystem::path& path)
{
    if (read_word(bytes, 0) != spirv_magic)
    {
        throw std::runtime_error{"invalid SPIR-V magic: " + path.string()};
    }

    if (read_word(bytes, 3) == 0)
    {
        throw std::runtime_error{"invalid SPIR-V identifier bound: " + path.string()};
    }
}

} // namespace

int main(int argc, char* argv[])
{
    try
    {
        if (argc != 3)
        {
            throw std::invalid_argument{"expected vertex and fragment shader paths"};
        }

        const std::filesystem::path vertex_path{argv[1]};

        const std::filesystem::path fragment_path{argv[2]};

        const std::vector<char> vertex = load_binary(vertex_path);

        const std::vector<char> fragment = load_binary(fragment_path);

        validate_spirv(vertex, vertex_path);

        validate_spirv(fragment, fragment_path);

        if (vertex == fragment)
        {
            throw std::runtime_error{"vertex and fragment shader binaries are identical"};
        }

        return EXIT_SUCCESS;
    }
    catch (const std::exception& error)
    {
        std::cerr << "Shader binary validation failed: " << error.what() << '\n';

        return EXIT_FAILURE;
    }
}
