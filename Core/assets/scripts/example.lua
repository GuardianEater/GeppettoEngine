-- This is an example script that moves an object in a circle
-- with a vertical oscillation.

local time = 0
local radius = math.random(10, 15)
local angularSpeed = math.random(1, 15) * 0.1
local verticalAmplitude = math.random(1, 15)
local verticalFrequency = math.random(1, 15) * 0.1
local first = true

function Initialize()
  --print("initialize")
end

function Update(dt)
  if first then
    self.Transform.position = vec3.new(50, 5, 0)
    first = false
  end

  
  time = time + dt
  local angle = angularSpeed * time

  self.RigidBody.velocity.x = -radius * angularSpeed * math.sin(angle)
  self.RigidBody.velocity.z = radius * angularSpeed * math.cos(angle)
  self.RigidBody.velocity.y = verticalAmplitude * verticalFrequency * math.cos(verticalFrequency * time)

  self.Transform.rotation.x = -radius * angularSpeed * math.sin(angle)
  self.Transform.rotation.z = radius * angularSpeed * math.cos(angle)
  self.Transform.rotation.y = verticalAmplitude * verticalFrequency * math.cos(verticalFrequency * time)

end

function Exit()
  --print("shutdown")
end