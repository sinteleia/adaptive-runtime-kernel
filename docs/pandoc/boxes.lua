-- boxes.lua
-- Converte i Div Pandoc con classi "remark" / "definition" in ambienti LaTeX
-- Richiede che nel header.tex esistano \newtcolorbox{remark}{...} e \newtcolorbox{definition}{...}

function Div(el)
  local classes = el.classes

  local function has_class(name)
    for _, c in ipairs(classes) do
      if c == name then return true end
    end
    return false
  end

  local env = nil
  if has_class("remark") then
    env = "remark"
  elseif has_class("definition") then
    env = "definition"
  end

  if env then
    local blocks = {}
    table.insert(blocks, pandoc.RawBlock("latex", "\\begin{" .. env .. "}"))
    for _, b in ipairs(el.content) do
      table.insert(blocks, b)
    end
    table.insert(blocks, pandoc.RawBlock("latex", "\\end{" .. env .. "}"))
    return blocks
  end

  return nil
end
