#include <stdio.h>//doar fisiere text ppm/pgm
#include <stdlib.h>
#include <string.h>
#include <math.h>

typedef struct{
    int r, g, b;
} rgb_t;

typedef struct{
    char tip[3];
    int x1, x2, y1, y2;
    int width, height;
    int scale;
    int **matrix;
    rgb_t **color;
} image_t;

void freememory(image_t *img){
    if(img->matrix){
        for(int i=0; i<img->height; i++) free(img->matrix[i]);
        free(img->matrix);
        img->matrix = NULL;
    }
    if(img->color){
        for(int i=0; i<img->height; i++) free(img->color[i]);
        free(img->color);
        img->color = NULL;
    }
}

void closefile(FILE* f)
{
    if(f!=NULL) fclose(f);
}

int pow2(int n)
{
    if(n<=0) return 0;
    return (n&(n-1))==0;
}

int chartoint(char *x)
{
    int n=0;
    for(int i=0; x[i]!='\0'; i++) n=n*10+(x[i]-'0');
    return n;
}


int **matrixalloc(int m, int n)
{
    int **p;
    p=(int**)malloc(m*sizeof(int*));
    if(!p) return NULL;
    for(int i=0; i<m; i++) p[i]=calloc(n, sizeof(int));
    return p;
}

rgb_t **coloralloc(int m, int n)
{
    rgb_t **p;
    p=(rgb_t **)malloc(m*sizeof(rgb_t *));
    if(!p) return NULL;
    for(int i=0; i<m; i++) p[i]=calloc(n, sizeof(rgb_t));
    return p;
}

// functie care sare peste comentarii
void readnextint(FILE *f, int *val){
    char buf[100];
    int ch;
    while (1) {
        ch = fgetc(f);
        if (ch == '#'){
            fgets(buf, sizeof(buf), f);
        } else if (ch != ' ' && ch != '\n' && ch != '\t' && ch != '\r') {
            ungetc(ch, f);
            if (fscanf(f, "%d", val) == 1){
                return;
            } else {
                printf("Error reading variable\n");
                return;
            }
        }
    }
}

void rotatieselectie90(image_t *img) {
    int size = img->x2 - img->x1;
    if (strcmp(img->tip, "P3") == 0) {
        rgb_t **tmp = coloralloc(size, size);

        for (int i=0; i<size; i++) {
            for (int j=0; j<size; j++) {
                tmp[j][size - 1 - i] = img->color[img->y1 + i][img->x1 + j];
            }
        }

        for (int i=0; i<size; i++) {
            for (int j=0; j<size; j++) {
                img->color[img->y1 + i][img->x1 + j] = tmp[i][j];
            }
            free(tmp[i]);
        }
        free(tmp);
    } else {
        int **tmp = matrixalloc(size, size);
        for (int i=0; i<size; i++) {
            for (int j=0; j<size; j++) {
                tmp[j][size - 1 - i] = img->matrix[img->y1 + i][img->x1 + j];
            }
        }
        for (int i=0; i<size; i++) {
            for (int j=0; j<size; j++) {
                img->matrix[img->y1 + i][img->x1 + j] = tmp[i][j];
            }
            free(tmp[i]);
        }
        free(tmp);
    }
}

int clamp(double val) {
    int res = (int)round(val);
    if (res < 0) return 0;
    if (res > 255) return 255;
    return res;
}

void apply_filter(image_t *img, double kernel[3][3]) {
    //aloc matrice temporala pt ca altfel nu pot lucra cu matricea originala(valorile se schimba in fiecare rulare din for)
    rgb_t **temp = coloralloc(img->height, img->width);
    for (int i = 0; i < img->height; i++) {
        for (int j = 0; j < img->width; j++) {
            temp[i][j] = img->color[i][j];
            if (i >= img->y1 && i < img->y2 && j >= img->x1 && j < img->x2) {
                if (i > 0 && i < img->height - 1 && j > 0 && j < img->width - 1) {
                    double r = 0, g = 0, b = 0;
                    for (int ki = -1; ki <= 1; ki++) {
                        for (int kj = -1; kj <= 1; kj++) {
                            r += img->color[i + ki][j + kj].r * kernel[ki + 1][kj + 1];
                            g += img->color[i + ki][j + kj].g * kernel[ki + 1][kj + 1];
                            b += img->color[i + ki][j + kj].b * kernel[ki + 1][kj + 1];
                        }
                    }
                    temp[i][j].r = clamp(r);
                    temp[i][j].g = clamp(g);
                    temp[i][j].b = clamp(b);
                }
            }
        }
    }

    for (int i=0; i<img->height; i++) {
        for (int j=0; j<img->width; j++) {
            img->color[i][j] = temp[i][j];
        }
        free(temp[i]);
    }
    free(temp);
}


FILE* loadfile(char file[], image_t *img)
{
    FILE *f=fopen(file, "r");
    if(f==NULL){
        printf("Failed to load %s\n", file);
        return NULL;
    }
    printf("Loaded %s\n", file);
    img->tip[0]=fgetc(f), img->tip[1]=fgetc(f), img->tip[2]='\0';
    readnextint(f, &img->width);
    readnextint(f, &img->height);
    /*fgetc(f);// scot \n
    char buf[100];
    do{
        fgets(buf, 100, f);
    }while(buf[0]=='#'); // acum buf are ultima linie care nu e comentariu (adica width si height)
    char *p=strtok(buf, " ");
    int i=0;
    while(p)
    {
        if(i==0) img->width=chartoint(p);
        else img->height=chartoint(p);
        i++;
        p=strtok(NULL, " ");
    }*/
    if (strcmp(img->tip, "P1") != 0) {//P2 si P3 au si valoarea maxima scale
        readnextint(f, &img->scale);
    }
    img->x1=0, img->y1=0, img->x2=img->width, img->y2=img->height;
    if(!strcmp(img->tip, "P1")){
        img->matrix=matrixalloc(img->height, img->width);
        for(int i=0; i<img->height; i++){
            for(int j=0; j<img->width; j++){
                fscanf(f, "%d", &img->matrix[i][j]);
            }
        }
    }
    else if(!strcmp(img->tip, "P2")){
        img->matrix=matrixalloc(img->height, img->width);
        for(int i=0; i<img->height; i++){
            for(int j=0; j<img->width; j++){
                fscanf(f, "%d", &img->matrix[i][j]);
            }
        }
    }
    else if(!strcmp(img->tip, "P3")){
        img->color=coloralloc(img->height, img->width);
        for(int i=0; i<img->height; i++){
            for(int j=0; j<img->width; j++){
                fscanf(f, "%d %d %d", &img->color[i][j].r, &img->color[i][j].g, &img->color[i][j].b);
            }
        }
    }
    return f;
}

void histogram(image_t img, int x, int y) {
    long frecv_pixel[256] = {0};
    for (int i = 0; i < img.height; i++) {
        for (int j = 0; j < img.width; j++) {
            frecv_pixel[img.matrix[i][j]]++;
        }
    }

    // grupez frecventele in cele Y bin-uri
    long *bins = calloc(y, sizeof(long));
    int valori_per_bin = 256 / y;//cate valori am per un singur bin

    for (int i=0; i<y; i++) {
        for (int j=0; j<valori_per_bin; j++) {
            bins[i] += frecv_pixel[i * valori_per_bin + j];
        }
    }

    long max_frecv_bin = 0;
    for (int i=0; i<y; i++) {
        if (bins[i] > max_frecv_bin) {
            max_frecv_bin = bins[i];
        }
    }

    for (int i = 0; i < y; i++) {
        int nr_stelute = 0;
        if (max_frecv_bin > 0) {
            nr_stelute = (bins[i] * x) / max_frecv_bin;
        }
        printf("%d\t|\t", nr_stelute);
        for (int j = 0; j < nr_stelute; j++) {
            printf("*");
        }
        printf("\n");
    }
    free(bins);
}

void histogram2(image_t img, int x, int y, int x1, int y1, int x2, int y2) {// DOAR pentru zona selectata
    long frecv_pixel[256] = {0};
    for (int i = y1; i < y2; i++) {
        for (int j = x1; j < x2; j++) {
            frecv_pixel[img.matrix[i][j]]++;
        }
    }

    long *bins = calloc(y, sizeof(long));
    int valori_per_bin = 256 / y;

    for (int i=0; i<y; i++) {
        for (int j=0; j<valori_per_bin; j++) {
            bins[i] += frecv_pixel[i * valori_per_bin + j];
        }
    }
    long max_frecv_bin = 0;
    for (int i=0; i<y; i++) {
        if (bins[i] > max_frecv_bin) max_frecv_bin = bins[i];
    }

    for (int i=0; i<y; i++) {
        int nr_stelute = 0;
        if (max_frecv_bin > 0) {
            nr_stelute = (bins[i] * x) / max_frecv_bin;
        }
        printf("%d\t|\t", nr_stelute);
        for (int j = 0; j < nr_stelute; j++) printf("*");
        printf("\n");
    }
    free(bins);
}


int main()
{
    FILE *f=NULL;
    int loaded=0;
    int x1=0, x2=0, y1=0, y2=0;
    char command[15], file[15];
    image_t img;
    img.matrix = NULL;
    img.color = NULL;
    double edge[3][3] = {{-1, -1, -1}, {-1, 8, -1}, {-1, -1, -1}};
    double sharpen[3][3] = {{0, -1, 0}, {-1, 5, -1}, {0, -1, 0}};
    double blur[3][3] = {{1/9.0, 1/9.0, 1/9.0}, {1/9.0, 1/9.0, 1/9.0}, {1/9.0, 1/9.0, 1/9.0}};
    double gaussian[3][3] = {{1/16.0, 2/16.0, 1/16.0}, {2/16.0, 4/16.0, 2/16.0}, {1/16.0, 2/16.0, 1/16.0}};
    while(1)
    {
        printf("Possible commands: LOAD, SELECT, HISTOGRAM, EQUALIZE, CROP, APPLY, ROTATE, CREDITS, SAVE, EXIT\nIntroduce the wanted command:\n");
        scanf("%s", command);
        if(strcmp(command, "LOAD")==0){
            printf("Enter the file name: \n");
            scanf("%s", file);
            if(loaded){
                freememory(&img);
                closefile(f);
            }
            f = loadfile(file, &img);
            if (f != NULL) {
                loaded = 1;
                x1 = img.x1; y1 = img.y1; x2 = img.x2; y2 = img.y2;
            } else {
                loaded = 0;
            }
        }
        else if(strcmp(command, "SELECT")==0){
            printf("Input 0 if you choose to restrict image area using x1, y1, x2, y2 or 1 to select the entire image\n");
            int choice;
            scanf("%d",&choice);
            if(choice==0)
            {
                printf("Enter x1, y1, x2, y2 coordinates\n");
                if(scanf("%d %d %d %d", &x1, &y1, &x2, &y2)==4)
                {
                    if(loaded==0){
                        printf("No image loaded\n");
                    }
                    else{
                        int aux;
                        if(x1 > x2) { aux = x1; x1 = x2; x2 = aux; } //in caz in care ordinea lor e stricata
                        if(y1 > y2) { aux = y1; y1 = y2; y2 = aux; }
                        if(x1 < 0 || y1 < 0 || x2 > img.width || y2 > img.height || x1 == x2 || y1 == y2) {
                            printf("Invalid set of coordinates\n");
                        }
                        else {
                            printf("Selected %d %d %d %d\n", x1, y1, x2, y2);
                            img.x1=x1, img.x2=x2, img.y1=y1, img.y2=y2;
                        }
                    }
                }
            }
            else if(choice==1){
                printf("Input ALL if you want to continue with this option: \n");
                char check[4];
                scanf("%s",check);
                if(strcmp(check, "ALL")==0)
                {
                    if(loaded==0) printf("No image loaded\n");
                    else{
                        img.x1=0, x1=img.x1, img.y1=0, y1=img.y1, img.x2=img.width, x2=img.x2, img.y2=img.height, y2=img.y2;
                        printf("Selected ALL\n");
                    }
                }
            }
            else printf("You did not input the right number\n");
        }
        else if(strcmp(command, "HISTOGRAM")==0){
            int x,y;
            if(loaded==0)
                printf("No image loaded\n");
            else{ printf("Will show histogram with at most X stars and Y bins. Introduce X and Y:\n");
                if(scanf("%d %d",&x, &y)==2)
                {
                    if(pow2(y)!=1) printf("Y must be a power of 2\n");
                    else
                    {
                        if(!strcmp(img.tip, "P3")) printf("Black and white image needed");
                        else histogram2(img, x, y, x1, y1, x2, y2);
                    }
                }
                else printf("Invalid set of parametres\n");
            }
        }
        else if (strcmp(command, "EQUALIZE") == 0) {
            if (loaded == 0) {
                printf("No image loaded\n");
            } else if (strcmp(img.tip, "P2") != 0) {
                printf("Image must be gray scale\n");
            } else {
                // Calculez H[i] pe toata imaginea
                long H[256] = {0};
                for (int i = 0; i < img.height; i++) {
                    for (int j = 0; j < img.width; j++) {
                        H[img.matrix[i][j]]++;
                    }
                }
                long S[256] = {0};
                S[0] = H[0];
                for (int i = 1; i <= 255; i++) {
                    S[i] = S[i - 1] + H[i];
                }
                //Aplica transformarea f(a) = (255 / Area) * S[a]
                double area = (double)img.width * img.height;
                for (int i = 0; i < img.height; i++) {
                    for (int j = 0; j < img.width; j++) {
                    int a = img.matrix[i][j]; // valoarea veche
                    double new_val = (255.0 / area) * (double)S[a];
                    int rounded_val = (int)round(new_val);//rotunjesc rezultatul folosind round()
                    // functia clamp pentru intervalul [0, 255]
                    if (rounded_val < 0) rounded_val = 0;
                    if (rounded_val > 255) rounded_val = 255;
                    img.matrix[i][j] = rounded_val;
                    }
                }
                printf("Equalize done\n");
            }
        }
        else if(strcmp(command, "SAVE")==0){
            if(loaded==0) printf("No image loaded\n");
            else{
                printf("Introduce filename desired (without extension) (example: text):\n");
                char name[15];//adaug .pgm sau .ppm in functie de tipul de img
                scanf("%s",name);
                if(strcmp(img.tip, "P3")==0) strcat(name, ".ppm");//color
                else strcat(name, ".pgm"); //alb negru sau grayscale
                FILE *g=fopen(name, "w+");
                if(g==NULL)
                {
                    printf("Error saving file\n");
                    return 0;
                }
                fprintf(g, "%s\n", img.tip), fprintf(g, "%d %d\n", img.width, img.height);
                if(strcmp(img.tip, "P3")!=0)
                {
                    if(strcmp(img.tip, "P2")==0) fprintf(g, "%d\n",img.scale);
                    for(int i=0; i<img.height; i++){
                        for(int j=0; j<img.width; j++){
                            fprintf(g, "%d ", img.matrix[i][j]);
                        }
                        fprintf(g, "\n");
                    }
                }
                else{
                    fprintf(g, "%d\n",img.scale);
                    for(int i=0; i<img.height; i++){
                        for(int j=0; j<img.width; j++){
                            fprintf(g, "%d %d %d ", img.color[i][j].r, img.color[i][j].g, img.color[i][j].b);
                        }
                        fprintf(g, "\n");
                    }
                }
                printf("Saved %s\n",name);
                fclose(g);
            }
        }
        else if(strcmp(command, "CROP")==0){
            if(loaded==0) printf("No image loaded\n");
            else{
                /*img.height=img.y2-img.y1, img.width=img.x2-img.x1;
                img.x1=0,x1=0,img.y1=0,y1=0,img.x2=img.width,x2=img.x2, img.y2=img.height, y2=img.y2;*/
                int new_w = x2 - x1;
                int new_h = y2 - y1;
                if (strcmp(img.tip, "P3") == 0) {
                    rgb_t **new_color = coloralloc(new_h, new_w);
                    for (int i=0; i<new_h; i++) {
                        for (int j=0; j<new_w; j++) {
                            new_color[i][j] = img.color[y1 + i][x1 + j];
                        }
                    }
                    for (int i=0; i<img.height; i++) free(img.color[i]);
                    free(img.color);
                    img.color = new_color;
                } else {
                    int **new_matrix = matrixalloc(new_h, new_w);
                    for (int i=0; i<new_h; i++) {
                        for (int j=0; j<new_w; j++) {
                            new_matrix[i][j] = img.matrix[y1 + i][x1 + j];
                        }
                    }
                    for (int i=0; i<img.height; i++) free(img.matrix[i]);
                    free(img.matrix);
                    img.matrix = new_matrix;
                }
                img.width = new_w; img.height = new_h;
                img.x1 = 0; img.y1 = 0; img.x2 = new_w; img.y2 = new_h;
                x1 = 0; y1 = 0; x2 = new_w; y2 = new_h;
                printf("Image cropped\n");
            }
        }
        else if(strcmp(command, "APPLY")==0){//tot ce e pe marginea matricei va ramane neschimbat. puteam sa aleg sa o fac negru in mod conventional
            if(loaded==0) printf("No image loaded\n");
            else{
                if(strcmp(img.tip,"P3")!=0) printf("Easy, Charlie Chaplin\n");
                else{
                    printf("Introduce desired command from the following: EDGE, SHARPEN, BLUR, GAUSSIAN_BLUR\n");
                    char verific[15];
                    scanf("%s",verific);
                    if(!strcmp(verific, "EDGE")){
                        apply_filter(&img, edge);
                        printf("APPLY %s done\n",verific);
                    }
                    else if(!strcmp(verific, "SHARPEN")){
                        apply_filter(&img, sharpen);
                        printf("APPLY %s done\n",verific);
                    }
                    else if(!strcmp(verific, "BLUR")){
                        apply_filter(&img, blur);
                        printf("APPLY %s done\n",verific);
                    }
                    else if(!strcmp(verific, "GAUSSIAN_BLUR")){
                        apply_filter(&img, gaussian);
                        printf("APPLY %s done\n",verific);
                    }
                    else printf("APPLY parameter invalid\n");
                }
            }
        }
        else if(strcmp(command, "ROTATE")==0){
            if(loaded==0) printf("No image loaded\n");
            else if(x2-x1 != y2-y1){
                printf("The selection must be square\n");
            }
            else{
                printf("Introduce wanted angle: \n");
                int angle;
                scanf("%d",&angle);
                if(angle%90!=0 || angle >360 || angle < -360) printf("Unsupported rotation angle\n");
                else{
                    int rotations = (angle / 90) % 4;
                    if (rotations < 0) rotations += 4;
                    for (int k = 0; k < rotations; k++) {
                        rotatieselectie90(&img);
                    }
                    printf("Rotated %d\n", angle);
                }
            }
        }
        else if(strcmp(command, "CREDITS")==0)
            printf("Rusanescu Gabriel made this program\n");
        else if(strcmp(command, "EXIT")==0){
            if(loaded==0) printf("No image loaded\n");
            printf("Exiting program\n");
            break;
        }
        else printf("Invalid command\n");
    }
    freememory(&img);
    closefile(f);
    return 0;
}
