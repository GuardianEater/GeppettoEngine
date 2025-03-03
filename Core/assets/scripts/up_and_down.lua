-- move an object up and down

Time = 0
Speed = 2

local first = true
function Update(dt)

  if first then
    self.Transform.position = vec3.new(2, 3, 15)
    first = false
  end

  Time = Time + dt

  local velocity = self.RigidBody.velocity
  local position = self.Transform.position

  velocity.x = 0
  velocity.y = Speed * math.sin(Time)
  velocity.z = 0

end