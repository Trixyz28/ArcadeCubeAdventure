
struct PipelineInstances;

struct Instance {
	std::string* id;
	int Mid;
	int NTx;
	int Iid;
	int* Tid;
	DescriptorSet **DS;
	std::vector<DescriptorSetLayout *> *D;
	int NDs;

	glm::mat4 Wm;
	PipelineInstances *PI;
};

struct PipelineRef {
	std::string* id;
	Pipeline* P;
	VertexDescriptor* VD;

	void init(const char* _id, Pipeline* _P, VertexDescriptor* _VD) {
		id = new std::string(_id);
		P = _P;
		VD = _VD;
	}
};

struct PipelineInstances {
	Instance* I;
	int InstanceCount;
	PipelineRef* PR;
};


class Scene {
	public:
	VertexDescriptor *VD;	

	DescriptorSet *DSGlobal;
	
	BaseProject *BP;

	// Models, textures and Descriptors (values assigned to the uniforms)
	// Please note that Model objects depends on the corresponding vertex structure
	// Models
	int ModelCount = 0;
	Model **M;
	std::unordered_map<std::string, int> MeshIds;
	std::unordered_map<std::string, std::vector<glm::vec3>> vecMap;
	std::unordered_map<std::string, BoundingBox> bbMap;

	// Textures
	int TextureCount = 0;
	Texture **T;
	std::unordered_map<std::string, int> TextureIds;
	
	// Descriptor sets and instances
	int InstanceCount = 0;
	Instance **I;
	std::unordered_map<std::string, int> InstanceIds;

	std::unordered_map<std::string, PipelineRef*> PipelineIds;
	int PipelineInstanceCount = 0;
	PipelineInstances* PI;

	void init(BaseProject *_BP, VertexDescriptor *VD,
		std::vector<PipelineRef>& PRs, std::string file) {
		BP = _BP;

		for (int i = 0; i < PRs.size(); i++) {
			PipelineIds[*PRs[i].id] = &PRs[i];
		}

		// Models, textures and Descriptors (values assigned to the uniforms)
		nlohmann::json js;
		std::ifstream ifs("models/scene.json");
		if (!ifs.is_open()) {
		  std::cout << "Error! Scene file not found!";
		  exit(-1);
		}
		try {
			std::cout << "Parsing JSON\n";
			ifs >> js;
			ifs.close();
			std::cout << "\n\n\nScene contains " << js.size() << " definitions sections\n\n\n";
			
			// MODELS
			nlohmann::json ms = js["models"];
			ModelCount = ms.size();
			std::cout << "Models count: " << ModelCount << "\n";

			M = (Model **)calloc(ModelCount, sizeof(Model *));
			for(int k = 0; k < ModelCount; k++) {
				MeshIds[ms[k]["id"]] = k;
				std::string MT = ms[k]["format"].template get<std::string>();
				M[k] = new Model();

				M[k]->init(BP, VD, ms[k]["model"], (MT[0] == 'O') ? OBJ : ((MT[0] == 'G') ? GLTF : MGCG), ms[k]["id"], vecMap);
				std::cout << "\n\nTHE ID IS:" << ms[k]["id"] << "\n\n";
			}
			
			// TEXTURES
			nlohmann::json ts = js["textures"];
			TextureCount = ts.size();
			std::cout << "Textures count: " << TextureCount << "\n";

			T = (Texture **)calloc(ModelCount, sizeof(Texture *));
			for(int k = 0; k < TextureCount; k++) {
				TextureIds[ts[k]["id"]] = k;
				T[k] = new Texture();

				T[k]->init(BP, ts[k]["texture"]);
			}

			// INSTANCES TextureCount
			nlohmann::json pis = js["instances"];
			PipelineInstanceCount = pis.size();
			std::cout << "Pipeline Instances count: " << PipelineInstanceCount << "\n";
			PI = (PipelineInstances*)calloc(PipelineInstanceCount, sizeof(PipelineInstances));
			InstanceCount = 0;

			for (int k = 0; k < PipelineInstanceCount; k++) {
				std::string Pid = pis[k]["pipeline"].template get<std::string>();
				PI[k].PR = PipelineIds[Pid];
				nlohmann::json is = pis[k]["elements"];
				PI[k].InstanceCount = is.size();
				std::cout << "Pipeline: " << Pid << ", Instances count : " << PI[k].InstanceCount << "\n";
				PI[k].I = (Instance*)calloc(PI[k].InstanceCount, sizeof(Instance));
				
				for (int j = 0; j < PI[k].InstanceCount; j++) {
					std::cout << k << "." << j << "\t" << is[j]["id"] << ", " << is[j]["model"] << "(" << MeshIds[is[j]["model"]] << "), {";
					PI[k].I[j].id = new std::string(is[j]["id"]);
					PI[k].I[j].Mid = MeshIds[is[j]["model"]];
					int NTextures = is[j]["texture"].size();
					PI[k].I[j].NTx = NTextures;
					PI[k].I[j].Tid = (int *)calloc(NTextures, sizeof(int));
					std::cout << "#" << NTextures;
					// more textures
					for (int h = 0; h < NTextures; h++) {
						PI[k].I[j].Tid[h] = TextureIds[is[j]["texture"][h]];
						std::cout << " " << is[j]["texture"][h] << "(" << PI[k].I[j].Tid[h] << ")";
					}
					std::cout << "}\n";
					nlohmann::json TMjson = is[j]["transform"];
					float TMj[16];
					for (int l = 0; l < 16; l++) { TMj[l] = TMjson[l]; }
					PI[k].I[j].Wm = glm::mat4(TMj[0], TMj[4], TMj[8], TMj[12], TMj[1], TMj[5], TMj[9], TMj[13], TMj[2], TMj[6], TMj[10], TMj[14], TMj[3], TMj[7], TMj[11], TMj[15]);

					PI[k].I[j].PI = &PI[k];
					PI[k].I[j].D = &PI[k].PR->P->D;
					PI[k].I[j].NDs = PI[k].I[j].D->size()-1; /* exclude DSLGlobal */
					BP->setsInPool += (PI[k].I[j].NDs); 
					for (int h = 0; h < PI[k].I[j].NDs; h++) {
						DescriptorSetLayout* DSL = (*PI[k].I[j].D)[h+1];
						int DSLsize = DSL->Bindings.size();

						for (int l = 0; l < DSLsize; l++) {
							if (DSL->Bindings[l].type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER) {
								BP->uniformBlocksInPool += 1;
							}
							else {
								BP->texturesInPool += 1;
							}
						}
					}

					InstanceCount++;
				}
			}
			std::cout << "Creating instances\n";
			I = (Instance**)calloc(InstanceCount, sizeof(Instance*));

			int i = 0;
			for (int k = 0; k < PipelineInstanceCount; k++) {
				for (int j = 0; j < PI[k].InstanceCount; j++) {
					I[i] = &PI[k].I[j];
					InstanceIds[*I[i]->id] = i;
					I[i]->Iid = i;

					i++;
				}
			}
			std::cout << i << " instances created\n";
		} catch (const nlohmann::json::exception& e) {
			std::cout << e.what() << '\n';
		}
		std::cout << "Leaving scene loading and creation\n";
	}


	void pipelinesAndDescriptorSetsInit(DescriptorSetLayout *DSL) {
		std::cout << "Scene DS init\n";

		/* DSGlobal initialization */
		DSGlobal = new DescriptorSet();
		DSGlobal->init(BP, DSL, {});

		for(int i = 0; i < InstanceCount; i++) {
			std::cout << "I: " << i << ", NTx: " << I[i]->NTx << ", NDs: " << I[i]->NDs << "\n";
			Texture** Tids = (Texture**)calloc(I[i]->NTx, sizeof(Texture*));
			for (int j = 0; j < I[i]->NTx; j++) {
				Tids[j] = T[I[i]->Tid[j]];
			}
			/* initialize DS of all the Instances */
			I[i]->DS = (DescriptorSet**)calloc(I[i]->NDs, sizeof(DescriptorSet*));
			for (int j = 0; j < I[i]->NDs; j++) {
				I[i]->DS[j] = new DescriptorSet();
				/* start from the DSL after DSLGlobal */
				I[i]->DS[j]->init(BP, (*I[i]->D)[j+1], Tids);
			}
			free(Tids); 
		}
		std::cout << "Scene DS init Done\n";
	}
	
	void pipelinesAndDescriptorSetsCleanup() {
		// Cleanup datasets
		DSGlobal->cleanup();
		free(DSGlobal);
		for (int i = 0; i < InstanceCount; i++) {
			for (int j = 0; j < I[i]->NDs; j++) {
				I[i]->DS[j]->cleanup();
				delete I[i]->DS[j];
			}
			free(I[i]->DS);
		}
	}

	void localCleanup() {
		// Cleanup textures
		for(int i = 0; i < TextureCount; i++) {
			T[i]->cleanup();
			delete T[i];
		}
		free(T);
		
		// Cleanup models
		for(int i = 0; i < ModelCount; i++) {
			M[i]->cleanup();
			delete M[i];
		}
		free(M);
		
		for(int i = 0; i < InstanceCount; i++) {
			delete I[i]->id;
			free(I[i]->Tid);
		}
		free(I);

		for (int i = 0; i < PipelineInstanceCount; i++) {
			free(PI[i].I);
		}
		free(PI);
	}
	
    void populateCommandBuffer(VkCommandBuffer commandBuffer, int currentImage) {
		for (int k = 0; k < PipelineInstanceCount; k++) {
			/* Pipelines Binding */
			Pipeline* P = PI[k].PR->P;
			P->bind(commandBuffer);
			for (int i = 0; i < PI[k].InstanceCount; i++) {

				M[PI[k].I[i].Mid]->bind(commandBuffer);
				/* DS Bingdings: DSGlobal, DS */
				DSGlobal->bind(commandBuffer, *P, 0, currentImage);
				for (int j = 0; j < PI[k].I[i].NDs; j++) { /* start from the DS after DSGlobal */
					PI[k].I[i].DS[j]->bind(commandBuffer, *P, j+1, currentImage);
				}
				vkCmdDrawIndexed(commandBuffer,
					static_cast<uint32_t>(M[PI[k].I[i].Mid]->indices.size()), 1, 0, 0, 0);
			}
		}
	}
};
    
