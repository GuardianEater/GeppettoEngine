-- This is an example script that moves an object in a circle
-- with a vertical oscillation.

Time = 0 -- global variables are exposed to the editor
Radius = math.random(10, 15)
AngularSpeed = math.random(1, 15) * 0.1
VerticalAmplitude = math.random(1, 15)
VerticalFrequency = math.random(1, 15) * 0.1
local somesuperextralongname = "hellothisisaverylongname" -- local variables are not exposed to the editor

local first = true

function Initialize()
  --print("initialize")
end

function Update(dt)
  if first then
    self.Transform.position = vec3.new(50, 5, 0)
    first = false
  end

  Time = Time + dt
  local angle = AngularSpeed * Time

  local rotation = self.Transform.rotation -- note table values are references
  local velocity = self.RigidBody.velocity

  velocity.x = -Radius * AngularSpeed * math.sin(angle)
  velocity.z = Radius * AngularSpeed * math.cos(angle)
  velocity.y = VerticalAmplitude * VerticalFrequency * math.cos(VerticalFrequency * Time)

  rotation.x = -Radius * AngularSpeed * math.sin(angle)
  rotation.z = Radius * AngularSpeed * math.cos(angle)
  rotation.y = VerticalAmplitude * VerticalFrequency * math.cos(VerticalFrequency * Time)

end

function Exit()
  --print("shutdown")
end