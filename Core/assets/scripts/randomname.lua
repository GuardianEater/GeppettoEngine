-- when assigned to an entity jumbles the name of the entity

local time = 0

function Update(dt)
  self.Identification.name = "Name is [" .. math.random(100000, 999999) .. "]"

  time = time + dt
end