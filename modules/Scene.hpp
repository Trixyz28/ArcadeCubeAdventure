
// Handler for the elements in the scene

struct PipelineInstances;

// Struct for instance
struct Instance {
	std::string* id;
	int modelId;
	int nTextures;
	int* textureVec;
	DescriptorSet **DS;
	int nDSs;	/*number of DS*/
	std::vector<DescriptorSetLayout *> *DSL;

	glm::mat4 Wm;
	PipelineInstances *PI;
};


// Struct for instance of pipeline
struct PipelineInstances {
	std::string* id;
	Pipeline* P;
	Instance* I;
	int InstanceCount;	/*number of instances of the current pipeline */

	void init(const char* _id, Pipeline* _P) {
		id = new std::string(_id);
		P = _P;
	}
};

// Main class
class Scene {
	public:

	VertexDescriptor *VD;
	DescriptorSet *DSGlobal;
	
	BaseProject *BP;

	// Models, textures and Descriptors (values assigned to the uniforms)
	// Model objects depends on the corresponding vertex structure
	

	// Models
	int modelCount = 0;
	Model **M;
	// Map the name of the model to its id
	std::unordered_map<std::string, int> modelIdMap;

	// Map the name of the model to its vertices
	std::unordered_map<std::string, std::vector<glm::vec3>> vecMap;


	// Textures
	int textureCount = 0;
	Texture **T;

	// Map the name of the texture to its id
	std::unordered_map<std::string, int> textureIdMap;
	

	// Instances
	int instanceCount = 0;
	Instance **I;
	std::unordered_map<std::string, int> instanceIdMap;

	// Map the name (id) of the instance to the bounding box
	std::unordered_map<std::string, BoundingBox> bbMap;


	// Pipelines
	// Map the name of the pipeline to PipelineRef
	std::unordered_map<std::string, Pipeline*> pipelineMap;

	int pipelineInstanceCount = 0;
	PipelineInstances *PI;


	// Initialization
	void init(BaseProject *_BP, VertexDescriptor *VD, std::vector<PipelineInstances>& PRs, std::string file) {
		
		BP = _BP;

		for (int i = 0; i < PRs.size(); i++) {
			pipelineMap[*PRs[i].id] = PRs[i].P;
		}

		// Open json file
		nlohmann::json js;
		std::ifstream ifs(file);
		if (!ifs.is_open()) {
		  std::cout << "Error! Scene file not found!";
		  exit(-1);
		}

		// Parse and load json file
		try {
			std::cout << "Parsing JSON\n";
			ifs >> js;
			ifs.close();
			std::cout << "\nScene contains " << js.size() << " definitions sections\n";
			

			// MODELS
			nlohmann::json models = js["models"];
			modelCount = models.size();
			std::cout << "Models count: " << modelCount << "\n";

			// Initialize pointers to models
			M = (Model **)calloc(modelCount, sizeof(Model *));

			for(int k = 0; k < modelCount; k++) {
				// Map the name (id) of the model to index (k)
				modelIdMap[models[k]["id"]] = k;

				// Initialize model
				M[k] = new Model();
				std::string modelType = models[k]["format"];
				M[k]->init(BP, VD, models[k]["model"], (modelType[0] == 'O') ? OBJ : ((modelType[0] == 'G') ? GLTF : MGCG), models[k]["id"], vecMap);
				std::cout << "\nThe model name is " << models[k]["id"] << "\n\n\n";
			}
			

			// TEXTURES
			nlohmann::json textures = js["textures"];
			textureCount = textures.size();
			std::cout << "Textures count: " << textureCount << "\n";

			// Initialize pointers to textures
			T = (Texture **)calloc(modelCount, sizeof(Texture *));

			for(int k = 0; k < textureCount; k++) {
				// Map the name of the texture to the index
				textureIdMap[textures[k]["id"]] = k;

				// Initialize texture
				T[k] = new Texture();
				T[k]->init(BP, textures[k]["texture"]);
			}


			// INSTANCES
			nlohmann::json pInstances = js["instances"];
			pipelineInstanceCount = pInstances.size();
			std::cout << "Pipeline Instances count: " << pipelineInstanceCount << "\n";

			// Initialize instances for each pipeline
			PI = (PipelineInstances*)calloc(pipelineInstanceCount, sizeof(PipelineInstances));
			instanceCount = 0;

			for (int k = 0; k < pipelineInstanceCount; k++) {

				std::string pipelineId = pInstances[k]["pipeline"];
				PI[k].id = &pipelineId;
				PI[k].P = pipelineMap[pipelineId];

				// Load elements for each pipeline
				nlohmann::json instances = pInstances[k]["elements"];
				PI[k].InstanceCount = instances.size();
				std::cout << "\nPipeline: " << pipelineId << ", Instances count : " << PI[k].InstanceCount << "\n";
				PI[k].I = (Instance*)calloc(PI[k].InstanceCount, sizeof(Instance));
				

				for (int j = 0; j < PI[k].InstanceCount; j++) {
					// Load parameters for instances
					std::cout << k << "." << j << "\t" << instances[j]["id"] << ", " << instances[j]["model"] << "(" << modelIdMap[instances[j]["model"]] << "), {";
					PI[k].I[j].id = new std::string(instances[j]["id"]);
					PI[k].I[j].modelId = modelIdMap[instances[j]["model"]];
					
					// Save textures
					int nTx = instances[j]["texture"].size();
					PI[k].I[j].nTextures = nTx;
					PI[k].I[j].textureVec = (int *)calloc(nTx, sizeof(int));
					std::cout << "#" << nTx;

					// More textures
					for (int h = 0; h < nTx; h++) {
						PI[k].I[j].textureVec[h] = textureIdMap[instances[j]["texture"][h]];
						std::cout << " " << instances[j]["texture"][h] << "(" << PI[k].I[j].textureVec[h] << ")";
					}
					std::cout << "}\n";


					// Create world matrices for instances

					// Scaling factors on x, y, z axis respectively
					nlohmann::json scalingFactors = instances[j]["scale"];
					
					// Rotation angles (in degrees) on x, y, z axis respectively
					nlohmann::json rotationAngles = instances[j]["rotate"];
					
					// Translation on x, y, z axis respectively
					nlohmann::json translatePos = instances[j]["translate"];

					float scalingVec[3], rotationVec[3], translateVec[3];
					for (int l = 0; l < 3; l++) { 
						scalingVec[l] = scalingFactors[l];
						rotationVec[l] = rotationAngles[l];
						translateVec[l] = translatePos[l];
					}

					// Transform order: scale, rotate z, rotate x, rotate y, translate
					PI[k].I[j].Wm = glm::translate(glm::mat4(1.0f), glm::vec3(translateVec[0], translateVec[1], translateVec[2]))
						* glm::rotate(glm::mat4(1.0f), glm::radians(rotationVec[1]), glm::vec3(0.0f, 1.0f, 0.0f))
						* glm::rotate(glm::mat4(1.0f), glm::radians(rotationVec[0]), glm::vec3(1.0f, 0.0f, 0.0f))
						* glm::rotate(glm::mat4(1.0f), glm::radians(rotationVec[2]), glm::vec3(0.0f, 0.0f, 1.0f))
						* glm::scale(glm::mat4(1.0f), glm::vec3(scalingVec[0], scalingVec[1], scalingVec[2]));
		

					// Link references pipeline - instance
					PI[k].I[j].PI = &PI[k];
					PI[k].I[j].DSL = &PI[k].P->D;
					PI[k].I[j].nDSs = PI[k].I[j].DSL->size()-1; /* exclude DSLGlobal */
					BP->setsInPool += (PI[k].I[j].nDSs);

					for (int h = 0; h < PI[k].I[j].nDSs; h++) {
						DescriptorSetLayout* DSL = (*PI[k].I[j].DSL)[h+1];	/* exclude DSLGlobal (do not need a DSLG for each instance) */
						int DSLsize = DSL->Bindings.size();

						// Count blocks in pool
						for (int l = 0; l < DSLsize; l++) {
							if (DSL->Bindings[l].type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER) {
								BP->uniformBlocksInPool += 1;
							}
							else {
								BP->texturesInPool += 1;
							}
						}
					}
					instanceCount++;
				}
			}

			// Create instances
			std::cout << "Creating instances\n";
			I = (Instance**)calloc(instanceCount, sizeof(Instance*));

			int i = 0;
			for (int k = 0; k < pipelineInstanceCount; k++) {
				for (int j = 0; j < PI[k].InstanceCount; j++) {
					// Load all instances in the map
					I[i] = &PI[k].I[j];
					instanceIdMap[*I[i]->id] = i;

					i++;
				}
			}
			std::cout << i << " instances created\n";
		} catch (const nlohmann::json::exception& e) {
			std::cout << e.what() << '\n';
		}

		std::cout << "Leaving scene loading and creation\n";

	}

	// Create Descriptor Sets
	void descriptorSetsInit(DescriptorSetLayout *DSL) {
		std::cout << "Scene DS init\n";

		/* DSGlobal initialization */
		DSGlobal = new DescriptorSet();
		DSGlobal->init(BP, DSL, {});

		// Create DS for each instance
		for(int i = 0; i < instanceCount; i++) {
			std::cout << "I: " << i << ", NTx: " << I[i]->nTextures << ", NDs: " << I[i]->nDSs << "\n";
			Texture** Tids = (Texture**)calloc(I[i]->nTextures, sizeof(Texture*));
			for (int j = 0; j < I[i]->nTextures; j++) {
				// Take textures for each instance
				Tids[j] = T[I[i]->textureVec[j]];
			}

			/* initialize DS of all the Instances */
			I[i]->DS = (DescriptorSet**)calloc(I[i]->nDSs, sizeof(DescriptorSet*));
			for (int j = 0; j < I[i]->nDSs; j++) {
				I[i]->DS[j] = new DescriptorSet();
				/* start from the DSL after DSLGlobal */
				I[i]->DS[j]->init(BP, (*I[i]->DSL)[j+1], Tids);
			}
			free(Tids); 
		}
		std::cout << "Scene DS init Done\n";
	}
	
	// Cleanup descriptor sets
	void descriptorSetsCleanup() {
		DSGlobal->cleanup();
		free(DSGlobal);
		for (int i = 0; i < instanceCount; i++) {
			for (int j = 0; j < I[i]->nDSs; j++) {
				I[i]->DS[j]->cleanup();
				delete I[i]->DS[j];
			}
			free(I[i]->DS);
		}
	}


	// Local cleanup
	void localCleanup() {
		// Cleanup textures
		for(int i = 0; i < textureCount; i++) {
			T[i]->cleanup();
			delete T[i];
		}
		free(T);
		
		// Cleanup models
		for(int i = 0; i < modelCount; i++) {
			M[i]->cleanup();
			delete M[i];
		}
		free(M);
		
		// Delete instances and references
		for(int i = 0; i < instanceCount; i++) {
			delete I[i]->id;
			free(I[i]->textureVec);
		}
		free(I);

		for (int i = 0; i < pipelineInstanceCount; i++) {
			free(PI[i].I);
		}
		free(PI);
	}
	

	// Creation of command buffer
    void populateCommandBuffer(VkCommandBuffer commandBuffer, int currentImage) {

		for (int k = 0; k < pipelineInstanceCount; k++) {

			/* Pipelines Binding */
			Pipeline* P = PI[k].P;
			P->bind(commandBuffer);

			// Bind Global Descriptor Set at set 0 for pipeline
			DSGlobal->bind(commandBuffer, *P, 0, currentImage);

			for (int i = 0; i < PI[k].InstanceCount; i++) {

				// Bind model
				M[PI[k].I[i].modelId]->bind(commandBuffer);

				/* DS Bindings at sets j+1 */
				for (int j = 0; j < PI[k].I[i].nDSs; j++) { /* start from the DS after DSGlobal */
					PI[k].I[i].DS[j]->bind(commandBuffer, *P, j+1, currentImage);
				}

				// Draw call for each instance
				vkCmdDrawIndexed(commandBuffer,
					static_cast<uint32_t>(M[PI[k].I[i].modelId]->indices.size()), 1, 0, 0, 0);
			}
		}
	}
};
    
