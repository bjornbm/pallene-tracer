-- vim: set filetype=lua : --

-- Luacheck configuration file [1]
--
-- Please install Luacheck to your system with Luarocks:
--
--    luarocks install luacheck
--
-- To run Luackeck, please use our linter script, which also runs additional checks:
--
--    ./run-lint
--
-- For vim integration, I recommend ALE [2]. It supports luacheck out of the box
-- For other editors such as Sublime, Atom, Emacs, Brackets, and VS Code,
-- consult the instructions in the Luacheck documentation [3].
--
-- [1] https://luacheck.readthedocs.io/en/stable/config.html
-- [2] https://github.com/dense-analysis/ale
-- [3] https://github.com/mpeterv/luacheck#editor-support

ignore = {
    "212/self", -- Unused argument "self"

    "411/ok",    -- Redefining local "ok"
    "411/errs?", -- Redefining local "err" or "errs"

    "421/ok",    -- Shadowing local "ok"
    "421/errs?", -- Shadowing local "err" or "errs"

    "542", -- Empty if branch.
    "6..", -- Whitespace warnings
}

files["spec"] = {
    std = "+busted"
}
