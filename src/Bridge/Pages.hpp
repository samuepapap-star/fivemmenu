#pragma once
#include <Includes/CustomWidgets/Custom.hpp>
#include <Bridge/Features.hpp>
#include <Bridge/ModelCatalog.hpp>
namespace BridgePages {
inline void Footer(){
 BridgeFeatures::Status();
 if(MenuBridge::Connection().Busy()&&Custom::Button("Cancelar acao",ImVec2(200,30),0))MenuBridge::Connection().Cancel();
}
inline void Render(int section){
 ImGui::SetCursorPos(ImVec2(184,136));ImGui::BeginChild("Acoes##Bridge",ImVec2(704,440),false,0);ImGui::PushID(section);
 if(section==0){
  ImGui::TextDisabled("Personagem / modelos e animais");
  static int selected=0;static char name[96]="a_c_chimp";
  const char* models[]={"a_c_chimp","a_c_mtlion","a_c_husky","a_c_cat_01","a_c_boar","a_c_cow","a_c_crow","s_m_y_cop_01","s_m_y_swat_01","mp_m_freemode_01","mp_f_freemode_01"};
  if(ImGui::Combo("Modelo predefinido",&selected,"Chimpanze\0Puma\0Cachorro\0Gato\0Javali\0Vaca\0Corvo\0Policial\0SWAT\0Freemode masculino\0Freemode feminino\0"))MenuBridge::Copy(name,sizeof(name),models[selected]);
  ImGui::InputText("Nome do modelo",name,sizeof(name));
  if(Custom::Button("Transformar personagem",ImVec2(-1,33),0))BridgeFeatures::Model(name);
  if(Custom::Button("Restaurar personagem original",ImVec2(-1,33),0))BridgeFeatures::Restore();
  ImGui::TextWrapped("A aparencia original e guardada antes da primeira troca. Saia do veiculo. A troca desativa God, NoClip e camera livre.");
 }
 if(section==1){
  ImGui::TextDisabled("Personagem / animacoes");
  static char dictionary[96]="missminuteman_1ig_2",animation[96]="handsup_base";static bool loop=true;
  static int selectedAnimation=0;
  const char* dictionaries[]={"missminuteman_1ig_2","anim@mp_player_intcelebrationmale@thumbs_up","anim@mp_player_intcelebrationmale@salute","anim@mp_player_intcelebrationmale@face_palm","amb@world_human_hang_out_street@male_c@idle_a"};
  const char* clips[]={"handsup_base","thumbs_up","salute","face_palm","idle_a"};
  if(ImGui::Combo("Escolher animacao",&selectedAnimation,"Maos ao alto\0Positivo\0Continencia\0Mao no rosto\0Esperar\0")){MenuBridge::Copy(dictionary,sizeof(dictionary),dictionaries[selectedAnimation]);MenuBridge::Copy(animation,sizeof(animation),clips[selectedAnimation]);}
  ImGui::InputText("Dicionario",dictionary,sizeof(dictionary));ImGui::InputText("Animacao",animation,sizeof(animation));Custom::CheckBox("Repetir",&loop);
  if(Custom::Button("Reproduzir animacao",ImVec2(-1,33),0))BridgeFeatures::Animate(dictionary,animation,loop);
  if(Custom::Button("Parar animacao",ImVec2(-1,33),0))BridgeFeatures::StopAnimation();
 }
 if(section==2){
  ImGui::TextDisabled("Carro / gerar veiculo");static char model[96]="adder";static bool network=false,enter=true;
  static const std::vector<std::string> defaults={"adder","zentorno","t20","sultan","blista","bati","faggio","bmx"};
  static std::vector<std::string> registered;static std::uint64_t cachedToken=0;static char search[96]{};
  const auto token=MenuBridge::Connection().CatalogToken();
  if(token!=cachedToken&&!(token&1u)){registered=MenuBridge::Connection().Models();cachedToken=token;}
  if(Custom::Button("Carregar lista de modelos",ImVec2(-1,30),0))BridgeFeatures::LoadVehicleCatalog();
  ImGui::TextDisabled(registered.empty()?"Predefinidos; carregue a lista completa do cliente":"Modelos registrados: %u",static_cast<unsigned>(registered.size()));
  ImGui::SetNextItemWidth(-1);ImGui::InputTextWithHint("##vehicle_filter","Buscar modelo na lista...",search,sizeof(search));
  const auto& models=registered.empty()?defaults:registered;
  const auto filter=MenuBridge::LowerModel(search);std::vector<std::size_t> filtered;
  ImGui::SetNextItemWidth(-1);
  if(ImGui::BeginCombo("##registered_vehicles",model,0)){
   for(std::size_t i=0;i<models.size();i++)if(models[i].find(filter)!=std::string::npos)filtered.push_back(i);
   ImGuiListClipper clipper;clipper.Begin(static_cast<int>(filtered.size()));
   while(clipper.Step())for(int i=clipper.DisplayStart;i<clipper.DisplayEnd;i++){
    const auto& code=models[filtered[static_cast<std::size_t>(i)]];
    if(ImGui::Selectable(code.c_str(),code==model))MenuBridge::Copy(model,sizeof(model),code);
   }
   if(filtered.empty())ImGui::TextDisabled("Nenhum modelo corresponde a busca");
   ImGui::EndCombo();
  }
  ImGui::SetNextItemWidth(360);ImGui::InputText("Nome personalizado",model,sizeof(model));
  Custom::CheckBox("Solicitar veiculo em rede",&network);Custom::CheckBox("Entrar no veiculo gerado",&enter);
  if(Custom::Button("Gerar veiculo",ImVec2(-1,33),0))BridgeFeatures::Spawn(model,network,enter);
  ImGui::TextWrapped("Sem rede, o veiculo e local. Em rede, a permanencia e sincronizacao dependem do servidor.");
 }
 Footer();ImGui::PopID();ImGui::EndChild();
}
}