target("Shaders")
    set_kind("phony")
    add_files("*.hlsl",
    "Clustered/*.hlsl")
    add_headerfiles("*.hlsl",
    "Clustered/*.hlsl")
target_end()