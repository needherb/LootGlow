-- include subprojects
includes("lib/commonlibob64")

-- set project constants
set_project("LootGlow")
set_version("0.4.2")
set_license("GPL-3.0")
set_languages("c++23")
set_warnings("allextra")

-- add common rules
add_rules("mode.debug", "mode.release", "mode.releasedbg")
add_rules("plugin.vsxmake.autoupdate")

-- define targets
target("LootGlow")
    add_rules("commonlibob64.plugin", {
        name = "LootGlow",
        author = "Elowen",
        description = "Highlights lootable containers in Oblivion Remastered"
    })

    -- add src files
    add_files("src/**.cpp")
    add_headerfiles("src/**.h")
    add_includedirs("src")
    set_pcxxheader("src/pch.h")

    if is_mode("release") then
        set_symbols("none")
        set_optimize("smallest")
        add_cxflags("/O1", "/Gy", "/Gw", { force = true })
        add_ldflags("/OPT:REF", "/OPT:ICF", "/DEBUG:NONE", { force = true })
    end
