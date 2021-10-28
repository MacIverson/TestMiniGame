// internal
#include "physics_system.hpp"
#include "world_init.hpp"

// Returns the local bounding coordinates scaled by the current size of the entity
vec2 get_bounding_box(const Motion& motion)
{
	// abs is to avoid negative scale due to the facing direction.
	return { abs(motion.scale.x), abs(motion.scale.y) };
}

// This is a SUPER APPROXIMATE check that puts a circle around the bounding boxes and sees
// if the center point of either object is inside the other's bounding-box-circle. You can
// surely implement a more accurate detection
bool collides(const Motion& motion1, const Motion& motion2)
{
	vec2 dp = motion1.position - motion2.position;
	float dist_squared = dot(dp,dp);
	const vec2 other_bonding_box = get_bounding_box(motion1) / 2.f;
	const float other_r_squared = dot(other_bonding_box, other_bonding_box);
	const vec2 my_bonding_box = get_bounding_box(motion2) / 2.f;
	const float my_r_squared = dot(my_bonding_box, my_bonding_box);
	const float r_squared = max(other_r_squared, my_r_squared);
	if (dist_squared < r_squared)
		return true;
	return false;
}

bool circlesIntersect(const Motion& motion1, const Motion& motion2) {
	float dist = sqrt(pow(motion1.position.x - motion2.position.x, 2) + pow(motion1.position.y - motion2.position.y, 2));
	if (dist < 50) {
		return true;
	}
	return false;
}

void handleMeshWallCollisions(Entity e, float window_height_px, float window_width_px) {
	Mesh* salmonMeshPointer = registry.meshPtrs.get(e);
	Motion& salmonMotion = registry.motions.get(e);
	Transform transform;
	transform.translate(salmonMotion.position);
	transform.rotate(salmonMotion.angle);
	transform.scale(salmonMotion.scale);

	for (int i = 0; i < salmonMeshPointer->vertices.size(); i++) {
		ColoredVertex vertex = salmonMeshPointer->vertices[i];
		vec3 pos = vertex.position;
		vec3 globalPos = transform.mat * pos;
		int globalY = globalPos.y + salmonMotion.position.y;
		int globalX = globalPos.x + salmonMotion.position.x;
		if (globalY < 0 && salmonMotion.velocity.y <= 0) {
			if (salmonMotion.velocity.y == 0) {
				salmonMotion.velocity.y = 100;
			}
			else {
				salmonMotion.velocity.y *= -1;
			}
		}
		if (globalY > window_height_px && salmonMotion.velocity.y >= 0) {
			if (salmonMotion.velocity.y == 0) {
				salmonMotion.velocity.y = -100;
			}
			else {
				salmonMotion.velocity.y *= -1;
			}
		}
		if (globalX < 0 || (globalX > window_width_px && salmonMotion.velocity.x > 0)) {
			salmonMotion.velocity.x *= -1;
		}
	}
}

void PhysicsSystem::step(float elapsed_ms, float window_width_px, float window_height_px)
{
	// Move fish based on how much time has passed, this is to (partially) avoid
	// having entities move at different speed based on the machine.
	auto& motion_registry = registry.motions;
	for(uint i = 0; i< motion_registry.size(); i++)
	{
		// !!! TODO A1: update motion.position based on step_seconds and motion.velocity
		Motion& motion = motion_registry.components[i];
		Entity entity = motion_registry.entities[i];
		float step_seconds = 1.0f * (elapsed_ms / 1000.f);
		motion.position += motion.velocity * step_seconds;
	}

	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// TODO A3: HANDLE PEBBLE UPDATES HERE
	// DON'T WORRY ABOUT THIS UNTIL ASSIGNMENT 3
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

	// Check for collisions between all moving entities
    ComponentContainer<Motion> &motion_container = registry.motions;
	for(uint i = 0; i<motion_container.components.size(); i++)
	{
		Motion& motion_i = motion_container.components[i];
		Entity entity_i = motion_container.entities[i];
		for(uint j = 0; j<motion_container.components.size(); j++) // i+1
		{
			if (i == j)
				continue;

			Motion& motion_j = motion_container.components[j];
			Entity entity_j = motion_container.entities[j];
			if (registry.softShells.has(entity_i) && registry.softShells.has(entity_j)) {
				// rocks are colliding, use circles to calcualte collisions
				if (circlesIntersect(motion_i, motion_j)) {
					registry.collisions.emplace_with_duplicates(entity_i, entity_j);
				}
			} 
			else if (collides(motion_i, motion_j)) {				
				// Create a collisions event
				// We are abusing the ECS system a bit in that we potentially insert muliple collisions for the same entity
				registry.collisions.emplace_with_duplicates(entity_i, entity_j);
				//registry.collisions.emplace_with_duplicates(entity_j, entity_i);
			}
		}
	}

	// handle rock - wall collisions here
	for (Entity e : registry.softShells.entities) {
		handleMeshWallCollisions(e, window_height_px, window_width_px);
	}
	// handle player - wall collisions here
	handleMeshWallCollisions(registry.players.entities[0], window_height_px, window_width_px);

	// you may need the following quantities to compute wall positions
	(float)window_width_px; (float)window_height_px;

	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// TODO A2: DRAW DEBUG INFO HERE on Salmon mesh collision
	// DON'T WORRY ABOUT THIS UNTIL ASSIGNMENT 2
	// You will want to use the createLine from world_init.hpp
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

	// debugging of bounding boxes
	if (debugging.in_debug_mode)
	{
		uint size_before_adding_new = (uint)motion_container.components.size();
		for (uint i = 0; i < size_before_adding_new; i++)
		{
			Motion& motion_i = motion_container.components[i];
			Entity entity_i = motion_container.entities[i];

			// visualize the radius with two axis-aligned lines
			const vec2 bonding_box = get_bounding_box(motion_i);
			float radius = sqrt(dot(bonding_box/2.f, bonding_box/2.f));
			vec2 line_scale1 = { motion_i.scale.x / 10, 2*radius };
			Entity line1 = createLine(motion_i.position, line_scale1);
			vec2 line_scale2 = { 2*radius, motion_i.scale.x / 10};
			Entity line2 = createLine(motion_i.position, line_scale2);

			// !!! TODO A2: implement debugging of bounding boxes and mesh
		}
	}

	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// TODO A3: HANDLE PEBBLE collisions HERE
	// DON'T WORRY ABOUT THIS UNTIL ASSIGNMENT 3
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
}